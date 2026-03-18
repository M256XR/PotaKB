// =========================================================================
// PotaKB Mouse fw v1
// MCP23017 キーマトリクスなし・マウスのみ
//
// 目的: キースキャンのI2Cオーバーヘッドを排除してマウスHzを計測・改善する
//
// ボタン:
//   BACK (D6) ホールド → スクロールモード
//   NEXT (D10) タップ  → BLEスロット切替
//
// LED:
//   緑固定       → USBモード
//   青固定/点滅  → BTモード (固定=接続中, 点滅=アドバタイズ中)
//
// シリアル (115200):
//   1秒ごとに "Hz=XXX" を出力 (マウスレポート送信頻度)
// =========================================================================

#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>
#include <InternalFileSystem.h>
#include "types.h"

using namespace Adafruit_LittleFS_Namespace;

// =========================================================================
// ピン定義
// =========================================================================
#define STICK_PHYS_X_PIN  A2   // PCBミス: 物理XピンがY動作
#define STICK_PHYS_Y_PIN  A3   // PCBミス: 物理YピンがX動作
#define BACK_BTN_PIN      D6
#define NEXT_BTN_PIN      D10
#define POWER_SW_PIN      D7
#define POWER_LED_PIN     D0
#define BT_LED_PIN        D1

#define DEBOUNCE_MS       5

// =========================================================================
// 設定構造体 (PotaKB_fw と同一バイナリ互換)
// =========================================================================
#pragma pack(push, 1)
struct Config {
  uint8_t  version;
  uint16_t stick_center_x;
  uint16_t stick_center_y;
  uint16_t stick_deadzone;
  uint16_t stick_range_x;
  uint16_t stick_range_y;
  float    stick_ema_alpha;
  float    mouse_max_speed;
  float    scroll_max_speed;
  uint32_t sleep_timeout_ms;
  uint8_t  led_brightness;
  uint16_t blink_interval_ms;
  uint8_t  scroll_invert;
  uint32_t magic;
};
#pragma pack(pop)

#define CONFIG_VERSION  3
#define CONFIG_MAGIC    0x504F5441UL

static const char* CONFIG_FILE = "/config.bin";
static const char* MODE_FILE   = "/mode.bin";

// デフォルト値
#define DEFAULT_STICK_CENTER_X    512
#define DEFAULT_STICK_CENTER_Y    512
#define DEFAULT_STICK_DEADZONE    40
#define DEFAULT_STICK_RANGE_X     450
#define DEFAULT_STICK_RANGE_Y     450
#define DEFAULT_STICK_EMA_ALPHA   0.4f
#define DEFAULT_MOUSE_MAX_SPEED   0.5f
#define DEFAULT_SCROLL_MAX_SPEED  0.02f
#define DEFAULT_SLEEP_TIMEOUT_MS  300000UL
#define DEFAULT_LED_BRIGHTNESS    50
#define DEFAULT_BLINK_INTERVAL_MS 500
#define DEFAULT_SCROLL_INVERT     0

// =========================================================================
// USB HID - hi-res マウス (Report ID 1)
// test_hiresscroll v5 と同一 descriptor
// =========================================================================
static const uint8_t desc_hid_usb[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,      //   Report ID (1)
    0x09, 0x01,      //   Usage (Pointer)
    0xA1, 0x00,      //   Collection (Physical)
      0x05, 0x09,    //     Usage Page (Button)
      0x19, 0x01, 0x29, 0x05,
      0x15, 0x00, 0x25, 0x01,
      0x95, 0x05, 0x75, 0x01, 0x81, 0x02,
      0x95, 0x01, 0x75, 0x03, 0x81, 0x03,
      0x05, 0x01,    //     Usage Page (Generic Desktop)
      0x09, 0x30, 0x09, 0x31,
      0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F,
      0x95, 0x02, 0x75, 0x10, 0x81, 0x06,
      0x09, 0x38,    //     Usage (Wheel)
      0xA1, 0x02,    //     Collection (Logical)
        0x05, 0x01, 0x09, 0x48,
        0x15, 0x00, 0x25, 0x01, 0x35, 0x01, 0x45, 0x78,
        0x95, 0x01, 0x75, 0x02, 0xB1, 0x02,
        0x95, 0x01, 0x75, 0x06, 0xB1, 0x03,
        0x05, 0x01, 0x09, 0x38,
        0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F,
        0x35, 0x00, 0x45, 0x00,
        0x95, 0x01, 0x75, 0x10, 0x81, 0x06,
      0xC0,
    0xC0,
  0xC0,
};

struct __attribute__((packed)) UsbMouseReport {
  uint8_t buttons;
  int16_t x, y, wheel;
};

Adafruit_USBD_HID usb_hid;
volatile uint8_t g_usb_res_mult = 1;  // 1 or 120

uint16_t usb_get_report_cb(uint8_t report_id, hid_report_type_t type,
                            uint8_t* buf, uint16_t req_len) {
  if (type == HID_REPORT_TYPE_FEATURE && report_id == 1) {
    buf[0] = (g_usb_res_mult == 120) ? 0x01 : 0x00;
    return 1;
  }
  return 0;
}

void usb_set_report_cb(uint8_t report_id, hid_report_type_t type,
                        uint8_t const* buf, uint16_t len) {
  if (type == HID_REPORT_TYPE_FEATURE && report_id == 1 && len >= 1) {
    g_usb_res_mult = ((buf[0] & 0x03) == 1) ? 120 : 1;
  }
}

// =========================================================================
// BLE
// =========================================================================
BLEDis        bledis;
BLEHidGeneric blehid(1, 0, 1);  // 1 input (mouse), 0 output, 1 feature (res multiplier)
BLEBas        bas;

volatile uint8_t g_ble_res_mult = 1;  // 1 or 120

void ble_feature_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void) conn_hdl; (void) chr;
  if (len >= 1) {
    g_ble_res_mult = ((data[0] & 0x03) == 1) ? 120 : 1;
    Serial.print("BLE hi-res: "); Serial.println(g_ble_res_mult);
  }
}

// =========================================================================
// グローバル変数
// =========================================================================
Config        currentConfig;
OperatingMode currentMode = MODE_USB;

float smoothed_x, smoothed_y;
float smooth_scroll_v = 0.0f;
unsigned long lastStickUpdateTime = 0;

float acc_mouse_x = 0.0f, acc_mouse_y = 0.0f;
float acc_scroll  = 0.0f, acc_pan     = 0.0f;
unsigned long lastMouseReportTime = 0;

bool backBtnState = false, nextBtnState = false;
bool lastRawBack  = false, lastRawNext  = false;
unsigned long backChangeTime = 0, nextChangeTime = 0;
bool nextWasPressed = false;  // エッジ検出

bool isSleeping = false;
unsigned long lastActivityTime = 0;
unsigned long lastBatteryTime  = 0;
int lastBatteryTier = -1;

// Hz 計測
uint32_t      hz_count = 0;
unsigned long hz_start = 0;

// BLE 接続間隔再交渉
unsigned long ble_connect_time  = 0;
unsigned long ble_last_param_req = 0;
bool          ble_param_needed  = false;

// =========================================================================
// 関数プロトタイプ
// =========================================================================
void loadDefaultConfig();
void loadConfig();
void saveConfig();
OperatingMode loadMode();
void saveMode(OperatingMode m);
void initHardware();
void initBLE();
void startAdvertising(uint8_t slot);
void switchMode(OperatingMode m);
void scanDirectKeys();
void updateStick(unsigned long now);
void sendMouseReport(unsigned long now);
void updateLED();
void updateBattery();
void goSleep();
void wakeUp();
void powerOff();
bool isAnyActivity();
void onConnect(uint16_t conn_handle);
void onDisconnect(uint16_t conn_handle, uint8_t reason);

// =========================================================================
// setup
// =========================================================================
void setup() {
  pinMode(POWER_SW_PIN, INPUT_PULLUP);
  if (digitalRead(POWER_SW_PIN) == HIGH) {
    NRF_POWER->SYSTEMOFF = 1;
    while (1);
  }

  Serial.begin(115200);

  TinyUSBDevice.setID(0x239A, 0x8029);
  TinyUSBDevice.setManufacturerDescriptor("0Re0_8192");
  TinyUSBDevice.setProductDescriptor("PotaKB Mouse");

  InternalFS.begin();
  loadDefaultConfig();
  loadConfig();
  currentMode = loadMode();

  initHardware();
  initBLE();
  updateBattery();

  {
    unsigned long t = millis();
    while (!TinyUSBDevice.mounted() && millis() - t < 2000) delay(10);
  }

  // USB 挿しても BT モードを維持 (USB は Serial ログ専用)
  // MODE_USB の時だけ BT1 にフォールバック
  if (currentMode == MODE_USB) {
    currentMode = MODE_BT1;
    saveMode(currentMode);
  }
  switchMode(currentMode);

  lastActivityTime    = millis();
  lastStickUpdateTime = millis();
  hz_start            = millis();
}

// =========================================================================
// loop
// =========================================================================
void loop() {
  if (digitalRead(POWER_SW_PIN) == HIGH) powerOff();

  // USB HID モードは NEXT ボタンで明示的に切替した時のみ

  unsigned long now = millis();

  scanDirectKeys();
  updateStick(now);
  sendMouseReport(now);

  // Hz ログ (1秒ごと)
  if (now - hz_start >= 1000) {
    float hz = (float)hz_count * 1000.0f / (float)(now - hz_start);
    Serial.print("Hz="); Serial.print((int)hz);
    if (Bluefruit.connected()) {
      uint16_t interval = Bluefruit.Connection(Bluefruit.connHandle())->getConnectionInterval();
      Serial.print(" interval="); Serial.print(interval);
      Serial.print("("); Serial.print(interval * 1.25f, 1); Serial.print("ms)");
    }
    Serial.println();
    hz_count = 0;
    hz_start = now;
  }

  // BLE 接続間隔の再交渉 (1秒後に初回、以降5秒ごとにリトライ)
  if (ble_param_needed && Bluefruit.connected()) {
    unsigned long since_connect = now - ble_connect_time;
    unsigned long since_req     = now - ble_last_param_req;
    bool do_req = (ble_last_param_req == 0 && since_connect > 1000) ||
                  (ble_last_param_req > 0   && since_req > 5000);
    if (do_req) {
      uint16_t hdl = Bluefruit.connHandle();
      Bluefruit.Connection(hdl)->requestConnectionParameter(6, 0, 200);
      ble_last_param_req = now;
      Serial.println("BLE param req sent");
    }
  }

  // スリープ管理 (BTモードのみ)
  if (currentMode != MODE_USB) {
    if (isSleeping) {
      if (isAnyActivity()) wakeUp();
      else { waitForEvent(); return; }
    }
    if (now - lastActivityTime > currentConfig.sleep_timeout_ms) goSleep();
  }

  if (now - lastBatteryTime > 60000UL) {
    updateBattery();
    lastBatteryTime = now;
  }

  updateLED();
}

// =========================================================================
// initHardware (MCP23017 なし)
// =========================================================================
void initHardware() {
  // LED
  pinMode(LED_RED,   OUTPUT); analogWrite(LED_RED,   255);
  pinMode(LED_GREEN, OUTPUT); analogWrite(LED_GREEN, 255);
  pinMode(LED_BLUE,  OUTPUT); analogWrite(LED_BLUE,  255);
  pinMode(POWER_LED_PIN, OUTPUT);
  analogWrite(POWER_LED_PIN, currentConfig.led_brightness);
  pinMode(BT_LED_PIN, OUTPUT);
  analogWrite(BT_LED_PIN, 0);

  // ボタン
  pinMode(BACK_BTN_PIN, INPUT_PULLUP);
  pinMode(NEXT_BTN_PIN, INPUT_PULLUP);
  nrf_gpio_cfg_sense_input(
    digitalPinToPinName(POWER_SW_PIN),
    NRF_GPIO_PIN_PULLUP,
    NRF_GPIO_PIN_SENSE_LOW
  );

  // USB HID
  usb_hid.setBootProtocol(HID_ITF_PROTOCOL_MOUSE);
  usb_hid.setPollInterval(1);
  usb_hid.setReportDescriptor(desc_hid_usb, sizeof(desc_hid_usb));
  usb_hid.setReportCallback(usb_get_report_cb, usb_set_report_cb);
  usb_hid.begin();

  // スムージング初期化
  smoothed_x = currentConfig.stick_center_x;
  smoothed_y = currentConfig.stick_center_y;
}

// =========================================================================
// initBLE
// =========================================================================
void initBLE() {
  // hvn_qsize=3: TX通知スロットを3に増やす (デフォルト1 → 66Hz → 改善狙い)
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.autoConnLed(false);
  Bluefruit.Periph.setConnInterval(6, 6);  // 7.5ms
  Bluefruit.setTxPower(4);
  Bluefruit.setName("PotaKB");
  Bluefruit.Periph.setConnectCallback(onConnect);
  Bluefruit.Periph.setDisconnectCallback(onDisconnect);

  bledis.setManufacturer("0Re0_8192");
  bledis.setModel("PotaKB Mouse v1");
  bledis.begin();

  {
    static uint16_t input_len[]   = { sizeof(UsbMouseReport) };
    static uint16_t feature_len[] = { 1 };
    blehid.setReportLen(input_len, NULL, feature_len);
    blehid.enableMouse(true);
    blehid.setReportMap(desc_hid_usb, sizeof(desc_hid_usb));
    blehid.setFeatureReportCallback(1, ble_feature_cb);
  }
  blehid.begin();
  bas.begin();
}

// =========================================================================
// startAdvertising
// =========================================================================
void startAdvertising(uint8_t btSlot) {
  Bluefruit.Advertising.clearData();
  Bluefruit.ScanResponse.clearData();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

// =========================================================================
// BLE コールバック
// =========================================================================
void onConnect(uint16_t conn_handle) {
  lastActivityTime = millis();
  lastBatteryTier  = -1;
  updateBattery();
  updateLED();

  // 接続後 1秒遅延してから要求 (Windows の GATT discovery 完了を待つ)
  ble_connect_time   = millis();
  ble_last_param_req = 0;
  ble_param_needed   = true;
}

void onDisconnect(uint16_t conn_handle, uint8_t reason) {
  lastActivityTime = millis();
  updateLED();
}

// =========================================================================
// switchMode
// =========================================================================
void switchMode(OperatingMode newMode) {
  if (currentMode != MODE_USB && Bluefruit.connected())
    Bluefruit.disconnect(Bluefruit.connHandle());
  Bluefruit.Advertising.stop();

  currentMode = newMode;
  saveMode(newMode);

  if (newMode == MODE_USB) {
    analogWrite(BT_LED_PIN, 0);
  } else {
    startAdvertising((uint8_t)(newMode - 1));
  }
  lastActivityTime = millis();
  updateLED();
}

// =========================================================================
// scanDirectKeys
// =========================================================================
void scanDirectKeys() {
  unsigned long now = millis();

  bool rawNext = (digitalRead(NEXT_BTN_PIN) == LOW);
  if (rawNext != lastRawNext) nextChangeTime = now;
  lastRawNext = rawNext;
  if (now - nextChangeTime > DEBOUNCE_MS) nextBtnState = rawNext;

  bool rawBack = (digitalRead(BACK_BTN_PIN) == LOW);
  if (rawBack != lastRawBack) backChangeTime = now;
  lastRawBack = rawBack;
  if (now - backChangeTime > DEBOUNCE_MS) backBtnState = rawBack;

  // NEXT: エッジ検出でモード切替
  if (nextBtnState && !nextWasPressed) {
    OperatingMode next = (OperatingMode)((currentMode + 1) % MODE_COUNT);
    switchMode(next);
  }
  nextWasPressed = nextBtnState;
}

// =========================================================================
// updateStick
// BACK ホールド → スクロールモード / 通常 → カーソルモード
// =========================================================================
void updateStick(unsigned long now) {
  float dt = (float)(now - lastStickUpdateTime);
  if (dt < 1.0f) return;
  lastStickUpdateTime = now;

  float raw_x = (float)analogRead(STICK_PHYS_Y_PIN);
  float raw_y = (float)analogRead(STICK_PHYS_X_PIN);

  float alpha = currentConfig.stick_ema_alpha;
  smoothed_x = alpha * raw_x + (1.0f - alpha) * smoothed_x;
  smoothed_y = alpha * raw_y + (1.0f - alpha) * smoothed_y;

  float cx = currentConfig.stick_center_x;
  float cy = currentConfig.stick_center_y;
  float dz = (float)currentConfig.stick_deadzone;

  float diff_x = smoothed_x - cx;
  float diff_y = smoothed_y - cy;
  if (fabsf(diff_x) < dz) diff_x = 0.0f;
  if (fabsf(diff_y) < dz) diff_y = 0.0f;

  auto calcSpeed = [&](float diff, float range) -> float {
    if (diff == 0.0f) return 0.0f;
    float norm = (fabsf(diff) - dz) / (range - dz);
    norm = constrain(norm, 0.0f, 1.0f);
    return copysignf(norm * norm, diff);
  };

  if (backBtnState) {
    // --- スクロールモード ---
    float sv_raw = calcSpeed(diff_y, (float)currentConfig.stick_range_y)
                   * currentConfig.scroll_max_speed * 1000.0f;

    if (diff_y != 0.0f) {
      smooth_scroll_v = alpha * sv_raw + (1.0f - alpha) * smooth_scroll_v;
    } else {
      smooth_scroll_v *= fmaxf(0.0f, 1.0f - 0.0035f * dt);
      if (fabsf(smooth_scroll_v) < 0.0001f) smooth_scroll_v = 0.0f;
    }

    float sign = currentConfig.scroll_invert ? 1.0f : -1.0f;
    acc_scroll += sign * smooth_scroll_v * dt * 0.001f;
    acc_scroll  = constrain(acc_scroll, -127.0f, 127.0f);
    acc_mouse_x = acc_mouse_y = 0.0f;

  } else {
    // --- カーソルモード ---
    float vx = calcSpeed(diff_x, (float)currentConfig.stick_range_x)
               * currentConfig.mouse_max_speed * 1000.0f;
    float vy = calcSpeed(diff_y, (float)currentConfig.stick_range_y)
               * currentConfig.mouse_max_speed * 1000.0f;

    acc_mouse_x += vx * dt * 0.001f;
    acc_mouse_y -= vy * dt * 0.001f;
    acc_mouse_x  = constrain(acc_mouse_x, -127.0f, 127.0f);
    acc_mouse_y  = constrain(acc_mouse_y, -127.0f, 127.0f);
    smooth_scroll_v = 0.0f;
  }
}

// =========================================================================
// sendMouseReport
// USB: 1ms 間隔 / BLE: 1ms 間隔 (接続イベントごとに届ける)
// =========================================================================
void sendMouseReport(unsigned long now) {
  if (now - lastMouseReportTime < 1) return;

  int8_t mx     = (int8_t)constrain((int)acc_mouse_x, -127, 127);
  int8_t my     = (int8_t)constrain((int)acc_mouse_y, -127, 127);
  int8_t scroll = (int8_t)constrain((int)acc_scroll,  -127, 127);

  bool hasMotion = (mx || my || scroll);
  if (!hasMotion) return;

  bool sent = false;

  if (currentMode == MODE_USB) {
    if (TinyUSBDevice.mounted() && usb_hid.ready()) {
      uint8_t mult = g_usb_res_mult;
      if (mult == 120) {
        // hi-res: 1/120 notch 単位
        int16_t wheel_val = (int16_t)constrain((int)(acc_scroll * 120.0f), -32767, 32767);
        UsbMouseReport rep = {0, (int16_t)(mx * 128), (int16_t)(my * 128), wheel_val};
        // mx/my は int8_t から int16_t にスケール (より細かい制御)
        // 実際にはそのまま送る
        rep.x = mx; rep.y = my;
        sent = usb_hid.sendReport(1, &rep, sizeof(rep));
        if (sent) {
          acc_mouse_x -= mx;
          acc_mouse_y -= my;
          acc_scroll  -= wheel_val / 120.0f;
        }
      } else {
        UsbMouseReport rep = {0, mx, my, scroll};
        sent = usb_hid.sendReport(1, &rep, sizeof(rep));
        if (sent) {
          acc_mouse_x -= mx;
          acc_mouse_y -= my;
          acc_scroll  -= scroll;
        }
      }
    }
  } else {
    // BLE
    if (Bluefruit.connected()) {
      if (g_ble_res_mult == 120) {
        int16_t wheel_val = (int16_t)constrain((int)(acc_scroll * 120.0f), -32767, 32767);
        UsbMouseReport rep = {0, mx, my, wheel_val};
        sent = blehid.inputReport(1, &rep, sizeof(rep));
        if (sent) {
          acc_mouse_x -= mx;
          acc_mouse_y -= my;
          acc_scroll  -= wheel_val / 120.0f;
        }
      } else {
        UsbMouseReport rep = {0, mx, my, scroll};
        sent = blehid.inputReport(1, &rep, sizeof(rep));
        if (sent) {
          acc_mouse_x -= mx;
          acc_mouse_y -= my;
          acc_scroll  -= scroll;
        }
      }
    }
  }

  if (sent) {
    lastMouseReportTime = now;
    hz_count++;
    lastActivityTime = now;
  }
}

// =========================================================================
// isAnyActivity
// =========================================================================
bool isAnyActivity() {
  if (backBtnState || nextBtnState) return true;
  float rx = (float)analogRead(STICK_PHYS_Y_PIN) - currentConfig.stick_center_x;
  float ry = (float)analogRead(STICK_PHYS_X_PIN) - currentConfig.stick_center_y;
  if (fabsf(rx) > currentConfig.stick_deadzone) return true;
  if (fabsf(ry) > currentConfig.stick_deadzone) return true;
  return false;
}

// =========================================================================
// スリープ・電源管理
// =========================================================================
void goSleep() {
  isSleeping = true;
  analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 255); analogWrite(LED_BLUE, 255);
  analogWrite(POWER_LED_PIN, 0);
  analogWrite(BT_LED_PIN, 0);
}

void wakeUp() {
  isSleeping = false;
  lastActivityTime = millis();
  analogWrite(POWER_LED_PIN, currentConfig.led_brightness);
  updateLED();
}

void powerOff() {
  analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 255); analogWrite(LED_BLUE, 255);
  analogWrite(POWER_LED_PIN, 0);
  analogWrite(BT_LED_PIN, 0);
  delay(50);
  NRF_POWER->SYSTEMOFF = 1;
}

// =========================================================================
// LED 更新
// =========================================================================
static unsigned long lastBlinkTime = 0;
static bool blinkState = false;

void updateLED() {
  if (isSleeping) return;
  switch (currentMode) {
    case MODE_USB:
      analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 0); analogWrite(LED_BLUE, 255);
      analogWrite(BT_LED_PIN, 0);
      break;
    case MODE_BT1: case MODE_BT2: case MODE_BT3:
      analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 255); analogWrite(LED_BLUE, 0);
      if (Bluefruit.connected()) {
        analogWrite(BT_LED_PIN, currentConfig.led_brightness);
      } else {
        unsigned long now = millis();
        if (now - lastBlinkTime > currentConfig.blink_interval_ms) {
          lastBlinkTime = now;
          blinkState = !blinkState;
          analogWrite(BT_LED_PIN, blinkState ? currentConfig.led_brightness : 0);
        }
      }
      break;
  }
}

// =========================================================================
// バッテリー
// =========================================================================
void updateBattery() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  delay(5);
  long sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogRead(PIN_VBAT); delay(1); }
  pinMode(VBAT_ENABLE, INPUT);
  float volt = (float)(sum / 16) / 1024.0f * 3.6f * (1510.0f / 510.0f);
  uint8_t pct;
  if      (volt <= 3.5f)  pct = 1;
  else if (volt <= 3.62f) pct = 3;
  else if (volt <= 3.8f)  pct = (uint8_t)((volt - 3.62f) * 277.78f + 5.0f);
  else if (volt <= 4.1f)  pct = (uint8_t)((volt - 3.8f)  * 150.0f  + 55.0f);
  else                    pct = 100;
  pct = constrain(pct, 1, 100);
  int tier = ((int)pct / 5) * 5;
  if (tier == 0) tier = 1;
  if (Bluefruit.connected() && tier != lastBatteryTier) {
    bas.notify((uint8_t)tier);
    lastBatteryTier = tier;
  }
}

// =========================================================================
// Flash ロード/セーブ
// =========================================================================
void loadDefaultConfig() {
  currentConfig.version           = CONFIG_VERSION;
  currentConfig.stick_center_x    = DEFAULT_STICK_CENTER_X;
  currentConfig.stick_center_y    = DEFAULT_STICK_CENTER_Y;
  currentConfig.stick_deadzone    = DEFAULT_STICK_DEADZONE;
  currentConfig.stick_range_x     = DEFAULT_STICK_RANGE_X;
  currentConfig.stick_range_y     = DEFAULT_STICK_RANGE_Y;
  currentConfig.stick_ema_alpha   = DEFAULT_STICK_EMA_ALPHA;
  currentConfig.mouse_max_speed   = DEFAULT_MOUSE_MAX_SPEED;
  currentConfig.scroll_max_speed  = DEFAULT_SCROLL_MAX_SPEED;
  currentConfig.sleep_timeout_ms  = DEFAULT_SLEEP_TIMEOUT_MS;
  currentConfig.led_brightness    = DEFAULT_LED_BRIGHTNESS;
  currentConfig.blink_interval_ms = DEFAULT_BLINK_INTERVAL_MS;
  currentConfig.scroll_invert     = DEFAULT_SCROLL_INVERT;
  currentConfig.magic             = CONFIG_MAGIC;
}

void loadConfig() {
  if (!InternalFS.exists(CONFIG_FILE)) return;
  File f = InternalFS.open(CONFIG_FILE, FILE_O_READ);
  if (!f) return;
  Config tmp;
  f.read((uint8_t*)&tmp, sizeof(Config));
  f.close();
  if (tmp.magic == CONFIG_MAGIC && tmp.version == CONFIG_VERSION) currentConfig = tmp;
}

void saveConfig() {
  InternalFS.remove(CONFIG_FILE);
  File f = InternalFS.open(CONFIG_FILE, FILE_O_WRITE);
  if (!f) return;
  f.write((uint8_t*)&currentConfig, sizeof(Config));
  f.close();
}

OperatingMode loadMode() {
  if (!InternalFS.exists(MODE_FILE)) return MODE_USB;
  File f = InternalFS.open(MODE_FILE, FILE_O_READ);
  if (!f) return MODE_USB;
  uint8_t m; f.read(&m, 1); f.close();
  if (m >= MODE_COUNT) return MODE_USB;
  return (OperatingMode)m;
}

void saveMode(OperatingMode m) {
  InternalFS.remove(MODE_FILE);
  File f = InternalFS.open(MODE_FILE, FILE_O_WRITE);
  if (!f) return;
  uint8_t v = (uint8_t)m;
  f.write(&v, 1);
  f.close();
}

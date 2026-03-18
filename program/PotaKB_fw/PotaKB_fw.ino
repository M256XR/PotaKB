// =========================================================================
// PotaKB Firmware v2.0
// XIAO nRF52840 + MCP23017
// =========================================================================

#include <bluefruit.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_TinyUSB.h>
#include <InternalFileSystem.h>
#include "keymap.h"

using namespace Adafruit_LittleFS_Namespace;

// =========================================================================
// 定数・ピン定義
// =========================================================================

#define STICK_PHYS_X_PIN  A2   // PCBミスにより物理XピンがY動作
#define STICK_PHYS_Y_PIN  A3   // PCBミスにより物理YピンがX動作

#define BACK_BTN_PIN      D6
#define NEXT_BTN_PIN      D10
#define POWER_SW_PIN      D7
#define POWER_LED_PIN     D0
#define BT_LED_PIN        D1

#define DEBOUNCE_MS       5

// BLEモード数
#define BT_SLOT_COUNT     3

// =========================================================================
// ファイル名
// =========================================================================
static const char* KEYMAP_FILE  = "/keymap.bin";
static const char* CONFIG_FILE  = "/config.bin";
static const char* MODE_FILE    = "/mode.bin";

// =========================================================================
// 設定構造体
// =========================================================================
#pragma pack(push, 1)
struct Config {
  uint8_t  version;
  uint16_t stick_center_x;
  uint16_t stick_center_y;
  uint16_t stick_deadzone;
  uint16_t stick_range_x;     // offset 7  スティックX軸の最大変位
  uint16_t stick_range_y;     // offset 9  スティックY軸の最大変位
  float    stick_ema_alpha;   // offset 11
  float    mouse_max_speed;   // offset 15
  float    scroll_max_speed;  // offset 19
  uint32_t sleep_timeout_ms;  // offset 23
  uint8_t  led_brightness;    // offset 27
  uint16_t blink_interval_ms; // offset 28
  uint32_t magic;             // offset 30 = 0x504F5441
};
#pragma pack(pop)

#define CONFIG_VERSION  2
#define CONFIG_MAGIC    0x504F5441UL  // "POTA"

// =========================================================================
// 動作モード
// =========================================================================
enum OperatingMode : uint8_t {
  MODE_USB = 0,
  MODE_BT1 = 1,
  MODE_BT2 = 2,
  MODE_BT3 = 3,
  MODE_COUNT = 4
};

// =========================================================================
// BLE UUID
// =========================================================================
#define KEYMAP_SVC_UUID   "adaf0001-c332-42a8-93bd-25e905756cb8"
#define KEYMAP_CHAR_UUID  "adaf0002-c332-42a8-93bd-25e905756cb8"
#define CONFIG_CHAR_UUID  "adaf0003-c332-42a8-93bd-25e905756cb8"

// =========================================================================
// グローバル変数
// =========================================================================

// --- ハードウェア ---
Adafruit_MCP23X17 mcp;

// --- HID ---
Adafruit_USBD_HID usb_hid;
static const uint8_t hid_desc[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
  TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2))
};

// --- BLE ---
BLEDis        bledis;
BLEHidAdafruit blehid;
BLEBas        bas;
BLEService    keymapSvc(KEYMAP_SVC_UUID);
BLECharacteristic keymapChar(KEYMAP_CHAR_UUID);
BLECharacteristic configChar(CONFIG_CHAR_UUID);

// --- 設定・キーマップ ---
Config currentConfig;
uint16_t current_keymap[NUM_LAYERS][LAYOUT_KEY_COUNT];

// --- モード ---
OperatingMode currentMode = MODE_USB;

// --- キーマトリクス ---
// matrix_to_layout[r][c] = レイアウトインデックス (-1=未割当)
int8_t matrix_to_layout[ROWS][COLS];

bool keyState[ROWS][COLS];
bool lastRawKeyState[ROWS][COLS];
unsigned long keyChangeTime[ROWS][COLS];

bool backBtnState = false, nextBtnState = false;
bool lastRawBack = false, lastRawNext = false;
unsigned long backChangeTime = 0, nextChangeTime = 0;

// --- レイヤー ---
uint8_t active_layer = 0;
bool layer_held[NUM_LAYERS];

// --- 前回レポート（変化検出用）---
uint8_t prev_kb_mod = 0;
uint8_t prev_kb_keys[6] = {0};
uint8_t prev_mouse_btn = 0;

// --- スティック（EMAスムージング）---
float smoothed_x = 512.0f;
float smoothed_y = 512.0f;
unsigned long lastStickUpdateTime = 0;

// --- マウスアキュムレータ ---
float acc_mouse_x = 0.0f;
float acc_mouse_y = 0.0f;
float acc_scroll  = 0.0f;
float acc_pan     = 0.0f;
unsigned long lastMouseReportTime = 0;

// --- モード切替フラグ ---
bool sw_mode_was_pressed = false;

// --- キャリブレーション中フラグ ---
bool calibrating = false;

// --- アクティビティ・スリープ ---
unsigned long lastActivityTime = 0;
bool isSleeping = false;

// --- バッテリー ---
unsigned long lastBatteryTime = 0;
int lastBatteryTier = -1;

// --- BLEデータ受信バッファ ---
uint8_t  keymap_buf[sizeof(current_keymap)];
uint16_t keymap_buf_offset = 0;
uint8_t  config_buf[sizeof(Config)];
uint16_t config_buf_offset = 0;

// --- シリアルコマンドバッファ ---
enum SerialState { SER_IDLE, SER_RECV_KEYMAP, SER_RECV_CONFIG };
SerialState serialState = SER_IDLE;
String serialCmdBuf = "";

// =========================================================================
// 関数プロトタイプ
// =========================================================================
void     loadDefaultConfig();
void     loadConfig();
void     saveConfig();
void     loadKeymap();
void     saveKeymap();
OperatingMode loadMode();
void     saveMode(OperatingMode m);

void     initHardware();
bool     initMCP();
void     initBLE();
void     startAdvertising(uint8_t btSlot);

void     scanMatrix();
void     scanDirectKeys();
void     processInputs();
void     updateStick(unsigned long now);
void     sendReports(uint8_t kb_mod, uint8_t kb_keys[6], uint8_t mouse_btn,
                     int8_t mx, int8_t my, int8_t scroll, int8_t pan);

void     switchMode(OperatingMode newMode);
void     updateLED();

void     doCalibrate();
void     doReset();
void     doReboot();

void     updateBattery();

void     handleSerial();

void     goSleep();
void     wakeUp();
void     powerOff();

bool     isAnyActivity();

// BLEコールバック
void onConnect(uint16_t conn_handle);
void onDisconnect(uint16_t conn_handle, uint8_t reason);
void onKeymapWrite(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
void onConfigWrite(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);

// =========================================================================
// setup
// =========================================================================
void setup() {
  Serial.begin(115200);

  TinyUSBDevice.setID(0x239A, 0x8029);
  TinyUSBDevice.setManufacturerDescriptor("0Re0_8192");
  TinyUSBDevice.setProductDescriptor("PotaKB");

  InternalFS.begin();

  loadDefaultConfig();
  loadConfig();
  loadKeymap();

  currentMode = loadMode();

  initHardware();
  initBLE();
  updateBattery();

  // USBマウント待ち（2秒）
  {
    unsigned long t = millis();
    while (!TinyUSBDevice.mounted() && millis() - t < 2000) delay(10);
  }

  // モード確定
  // USB接続あり → 保存モードに関わらずUSB優先
  if (TinyUSBDevice.mounted()) {
    currentMode = MODE_USB;
  } else if (currentMode == MODE_USB) {
    currentMode = MODE_BT1;
    saveMode(currentMode);
  }
  switchMode(currentMode);

  lastActivityTime = millis();
  lastStickUpdateTime = millis();
}

// =========================================================================
// loop
// =========================================================================
void loop() {
  // 電源スイッチ
  if (digitalRead(POWER_SW_PIN) == HIGH) powerOff();

  // USBケーブル抜き検出（モードは保存しない：次回USB接続時にUSBで起動できるよう）
  if (currentMode == MODE_USB && !TinyUSBDevice.mounted()) {
    currentMode = MODE_BT1;
    Bluefruit.Advertising.stop();
    startAdvertising(0);
    updateLED();
  }

  // シリアルコマンド処理
  if (Serial.available()) handleSerial();

  // キースキャン
  scanMatrix();
  scanDirectKeys();

  // 入力処理・レポート送信
  processInputs();

  // スリープ管理（BTモードのみ）
  if (currentMode != MODE_USB) {
    if (isSleeping) {
      if (isAnyActivity()) wakeUp();
      else waitForEvent();
      return;
    }
    if (millis() - lastActivityTime > currentConfig.sleep_timeout_ms) {
      goSleep();
    }
  }

  // バッテリー更新
  if (millis() - lastBatteryTime > 60000UL) {
    updateBattery();
    lastBatteryTime = millis();
  }

  updateLED();
}

// =========================================================================
// initHardware
// =========================================================================
void initHardware() {
  // LED
  pinMode(LED_BUILTIN, INPUT);
  pinMode(LED_RED,   OUTPUT); pinMode(LED_GREEN, OUTPUT); pinMode(LED_BLUE, OUTPUT);
  analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 255); analogWrite(LED_BLUE, 255);

  pinMode(POWER_LED_PIN, OUTPUT);
  analogWrite(POWER_LED_PIN, currentConfig.led_brightness);

  pinMode(BT_LED_PIN, OUTPUT);
  analogWrite(BT_LED_PIN, 0);

  // ボタン・スイッチ
  pinMode(BACK_BTN_PIN, INPUT_PULLUP);
  pinMode(NEXT_BTN_PIN, INPUT_PULLUP);
  pinMode(POWER_SW_PIN, INPUT_PULLUP);
  nrf_gpio_cfg_sense_input(
    digitalPinToPinName(POWER_SW_PIN),
    NRF_GPIO_PIN_PULLUP,
    NRF_GPIO_PIN_SENSE_LOW
  );

  // USB HID
  usb_hid.setPollInterval(1);
  usb_hid.setReportDescriptor(hid_desc, sizeof(hid_desc));
  usb_hid.begin();

  // MCP23017
  if (!initMCP()) {
    // 初期化失敗: 赤LED高速点滅でエラー通知
    Serial.println("ERROR: MCP23017 init failed");
    for (int i = 0; i < 10; i++) {
      analogWrite(LED_RED, 0); delay(100);
      analogWrite(LED_RED, 255); delay(100);
    }
  }
  Wire.setClock(400000);

  // マトリクス → レイアウトインデックス変換テーブル構築
  buildMatrixMap();

  // スムージング初期化
  smoothed_x = currentConfig.stick_center_x;
  smoothed_y = currentConfig.stick_center_y;
}

// =========================================================================
// マトリクスマップ構築
// sw番号(1-64) → マトリクス座標 の変換テーブル
// =========================================================================
void buildMatrixMap() {
  memset(matrix_to_layout, -1, sizeof(matrix_to_layout));

  // sw番号(1-based) → [row, col]
  static const uint8_t sw_to_rc[64][2] = {
    {0,0},{0,1},{0,2},{0,3},{1,1},{1,2},{1,3},{0,4},
    {2,1},{2,2},{2,3},{1,4},{3,1},{3,2},{3,3},{2,4},
    {4,1},{4,2},{4,3},{3,4},{5,1},{5,2},{5,3},{4,4},
    {6,1},{6,2},{6,3},{5,4},{4,5},{7,1},{7,2},{7,3},
    {6,4},{5,5},{1,0},{1,5},{3,6},{7,4},{6,5},{2,0},
    {2,5},{0,7},{4,7},{4,6},{3,0},{3,5},{1,7},{5,7},
    {5,6},{4,0},{0,6},{2,7},{6,7},{6,6},{5,0},{1,6},
    {7,5},{7,7},{6,0},{2,6},{3,7},{7,0},{0,5},{0,0}
  };

  // 物理レイアウト順（sw番号 1-based、63個）
  static const uint8_t layout_order[LAYOUT_KEY_COUNT - 2] = {
     1, 62,  2,  5,  9, 13, 17, 21, 25, 30, 35, 40, 45, 50, 55,
    59, 63,  3,  6, 10, 14, 18, 22, 26, 31, 36, 41, 46, 51, 56,
    60,  4,  7, 11, 15, 19, 23, 27, 32, 37, 42, 47, 52, 57, 61,
     8, 12, 16, 20, 24, 28, 33, 38, 43, 48, 53, 58, 29, 34, 39,
    44, 49, 54
  };

  for (int i = 0; i < (LAYOUT_KEY_COUNT - 2); i++) {
    int sw = layout_order[i];
    int r  = sw_to_rc[sw - 1][0];
    int c  = sw_to_rc[sw - 1][1];
    matrix_to_layout[r][c] = i;
  }
}

// =========================================================================
// MCP23017初期化
// =========================================================================
bool initMCP() {
  // I2Cバスリカバリ: SDAがLOWで詰まっている場合に9クロックで解放
  {
    const int SDA_PIN = SDA;
    const int SCL_PIN = SCL;
    pinMode(SCL_PIN, OUTPUT);
    pinMode(SDA_PIN, INPUT_PULLUP);
    if (digitalRead(SDA_PIN) == LOW) {
      for (int i = 0; i < 9; i++) {
        digitalWrite(SCL_PIN, HIGH); delayMicroseconds(5);
        digitalWrite(SCL_PIN, LOW);  delayMicroseconds(5);
      }
      // STOP condition
      pinMode(SDA_PIN, OUTPUT);
      digitalWrite(SDA_PIN, LOW);  delayMicroseconds(5);
      digitalWrite(SCL_PIN, HIGH); delayMicroseconds(5);
      digitalWrite(SDA_PIN, HIGH); delayMicroseconds(5);
    }
    // Wire に引き渡す
    Wire.begin();
  }

  if (!mcp.begin_I2C()) return false;
  for (int i = 0; i < 8;  i++) mcp.pinMode(i, INPUT_PULLUP);
  for (int i = 8; i < 16; i++) mcp.pinMode(i, OUTPUT);
  mcp.writeGPIOAB(0xFFFF);
  return true;
}

// =========================================================================
// scanMatrix
// =========================================================================
void scanMatrix() {
  unsigned long now = millis();
  for (int c = 0; c < COLS; c++) {
    mcp.writeGPIOAB(~(1u << (c + 8)));
    delayMicroseconds(5);
    uint16_t gpio = mcp.readGPIOAB();
    for (int r = 0; r < ROWS; r++) {
      bool raw = !(gpio & (1u << r));
      if (raw != lastRawKeyState[r][c]) keyChangeTime[r][c] = now;
      lastRawKeyState[r][c] = raw;
      if (now - keyChangeTime[r][c] > DEBOUNCE_MS) keyState[r][c] = raw;
    }
  }
  mcp.writeGPIOAB(0xFFFF);
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
}

// =========================================================================
// レイヤー解決
// キーコードを取得する（KC_TRNSなら下のレイヤーに透過）
// =========================================================================
uint16_t resolveKey(uint8_t layer, int idx) {
  uint16_t key = current_keymap[layer][idx];
  if (key == KC_TRNS && layer > 0) {
    key = current_keymap[0][idx];
  }
  return key;
}

// =========================================================================
// processInputs
// =========================================================================
void processInputs() {
  unsigned long now = millis();

  uint8_t kb_mod = 0;
  uint8_t kb_keys[6] = {0};
  uint8_t kb_count = 0;
  uint8_t mouse_btn = 0;
  bool sw_mode_now = false;
  bool activity = false;

  // --- レイヤー判定 ---
  // MO(n)キーが押されていたらそのレイヤーをアクティブに
  memset(layer_held, 0, sizeof(layer_held));
  layer_held[0] = true;  // Base常時アクティブ

  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (!keyState[r][c]) continue;
      int idx = matrix_to_layout[r][c];
      if (idx < 0) continue;
      uint16_t key = current_keymap[0][idx];
      if ((key & 0xFF00) == 0x5000) {
        uint8_t ln = key & 0x00FF;
        if (ln < NUM_LAYERS) layer_held[ln] = true;
      }
    }
  }
  // 直接キーも確認
  if (nextBtnState) {
    uint16_t k = current_keymap[0][63];
    if ((k & 0xFF00) == 0x5000 && (k & 0x00FF) < NUM_LAYERS)
      layer_held[k & 0x00FF] = true;
  }
  if (backBtnState) {
    uint16_t k = current_keymap[0][64];
    if ((k & 0xFF00) == 0x5000 && (k & 0x00FF) < NUM_LAYERS)
      layer_held[k & 0x00FF] = true;
  }

  // アクティブレイヤー = 最も番号が大きいheld layer
  active_layer = 0;
  for (int l = NUM_LAYERS - 1; l >= 0; l--) {
    if (layer_held[l]) { active_layer = l; break; }
  }

  // --- キー処理 ---
  auto processKey = [&](uint16_t key) {
    if (key == KC_NO || key == KC_TRNS) return;
    if ((key & 0xFF00) == 0x5000) return;  // MO() はここでは無視

    switch (key) {
      case SW_MODE:
        sw_mode_now = true;
        break;
      case KC_CALIBRATE:
        doCalibrate();
        break;
      case KC_RESET:
        doReset();
        break;
      case KC_REBOOT:
        doReboot();
        break;
      case KC_MS_UP:    acc_mouse_y -= currentConfig.mouse_max_speed; break;
      case KC_MS_DOWN:  acc_mouse_y += currentConfig.mouse_max_speed; break;
      case KC_MS_LEFT:  acc_mouse_x -= currentConfig.mouse_max_speed; break;
      case KC_MS_RIGHT: acc_mouse_x += currentConfig.mouse_max_speed; break;
      case KC_MS_BTN1:  mouse_btn |= MOUSE_BUTTON_LEFT;     break;
      case KC_MS_BTN2:  mouse_btn |= MOUSE_BUTTON_RIGHT;    break;
      case KC_MS_BTN3:  mouse_btn |= MOUSE_BUTTON_MIDDLE;   break;
      case KC_MS_BTN4:  mouse_btn |= MOUSE_BUTTON_BACKWARD; break;
      case KC_MS_BTN5:  mouse_btn |= MOUSE_BUTTON_FORWARD;  break;
      case KC_MS_SCR_UP:    acc_scroll -= 1.0f; break;
      case KC_MS_SCR_DOWN:  acc_scroll += 1.0f; break;
      case KC_MS_SCR_LEFT:  acc_pan   -= 1.0f; break;
      case KC_MS_SCR_RIGHT: acc_pan   += 1.0f; break;
      default:
        // 修飾キー
        if (key >= HID_KEY_CONTROL_LEFT && key <= HID_KEY_GUI_RIGHT) {
          kb_mod |= (1 << (key - HID_KEY_CONTROL_LEFT));
        } else if (kb_count < 6) {
          kb_keys[kb_count++] = (uint8_t)key;
        }
        break;
    }
  };

  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (!keyState[r][c]) continue;
      int idx = matrix_to_layout[r][c];
      if (idx < 0) continue;
      processKey(resolveKey(active_layer, idx));
    }
  }
  if (nextBtnState) processKey(resolveKey(active_layer, 63));
  if (backBtnState) processKey(resolveKey(active_layer, 64));

  // SW_MODEはエッジ検出（押した瞬間だけ）
  if (sw_mode_now && !sw_mode_was_pressed) {
    OperatingMode nextMode = (OperatingMode)((currentMode + 1) % MODE_COUNT);
    switchMode(nextMode);
  }
  sw_mode_was_pressed = sw_mode_now;

  // --- アナログスティック ---
  updateStick(now);

  // --- マウスレポート送信 ---
  // USB: 1ms間隔, BT: 7.5ms間隔
  uint16_t mouse_interval = (currentMode == MODE_USB) ? 1 : 8;
  bool sendMouse = (now - lastMouseReportTime >= mouse_interval);

  int8_t mx = 0, my = 0, scroll = 0, pan = 0;
  if (sendMouse) {
    lastMouseReportTime = now;
    mx = (int8_t)constrain((int)acc_mouse_x, -127, 127);
    my = (int8_t)constrain((int)acc_mouse_y, -127, 127);
    acc_mouse_x -= mx;
    acc_mouse_y -= my;
    scroll = (int8_t)constrain((int)acc_scroll, -127, 127);
    pan    = (int8_t)constrain((int)acc_pan,    -127, 127);
    acc_scroll -= scroll;
    acc_pan    -= pan;
  }

  // --- キーボードレポート送信（変化時のみ）---
  if (kb_mod != prev_kb_mod || memcmp(kb_keys, prev_kb_keys, 6) != 0) {
    if (currentMode == MODE_USB) {
      if (TinyUSBDevice.mounted() && usb_hid.ready())
        usb_hid.keyboardReport(1, kb_mod, kb_keys);
    } else {
      if (Bluefruit.connected()) blehid.keyboardReport(kb_mod, kb_keys);
    }
    prev_kb_mod = kb_mod;
    memcpy(prev_kb_keys, kb_keys, 6);
    activity = true;
  }

  // --- マウスレポート送信 ---
  if (sendMouse) {
    bool hasMotion = (mx || my || scroll || pan || mouse_btn || mouse_btn != prev_mouse_btn);
    if (hasMotion) {
      if (currentMode == MODE_USB) {
        if (TinyUSBDevice.mounted() && usb_hid.ready())
          usb_hid.mouseReport(2, mouse_btn, mx, my, scroll, pan);
      } else {
        if (Bluefruit.connected())
          blehid.mouseReport(mouse_btn, mx, my, scroll, pan);
      }
      prev_mouse_btn = mouse_btn;
      activity = true;
    }
  }

  if (activity) lastActivityTime = now;
}

// =========================================================================
// updateStick
// EMAスムージング + deltaTime速度計算
// =========================================================================
void updateStick(unsigned long now) {
  if (calibrating) return;  // キャリブレーション中はスティック無効
  float dt = (float)(now - lastStickUpdateTime);  // ms
  if (dt < 1.0f) return;
  lastStickUpdateTime = now;

  // 読み取り（PCBミス補正: A2→論理Y, A3→論理X）
  float raw_x = (float)analogRead(STICK_PHYS_Y_PIN);  // A3 → 論理X
  float raw_y = (float)analogRead(STICK_PHYS_X_PIN);  // A2 → 論理Y

  // EMAスムージング
  float alpha = currentConfig.stick_ema_alpha;
  smoothed_x = alpha * raw_x + (1.0f - alpha) * smoothed_x;
  smoothed_y = alpha * raw_y + (1.0f - alpha) * smoothed_y;

  float cx = currentConfig.stick_center_x;
  float cy = currentConfig.stick_center_y;
  float dz = currentConfig.stick_deadzone;

  float diff_x = smoothed_x - cx;
  float diff_y = smoothed_y - cy;

  // デッドゾーン
  if (fabsf(diff_x) < dz) diff_x = 0.0f;
  if (fabsf(diff_y) < dz) diff_y = 0.0f;

  if (active_layer == 0) {
    // --- マウスモード ---
    // 速度カーブ: 二乗（デッドゾーン付近は遅く、端は速い）
    auto calcSpeed = [&](float diff, float range) -> float {
      if (diff == 0.0f) return 0.0f;
      float norm = (fabsf(diff) - dz) / (range - dz);
      norm = constrain(norm, 0.0f, 1.0f);
      return copysignf(norm * norm * currentConfig.mouse_max_speed, diff);
    };

    float vx = calcSpeed(diff_x, (float)currentConfig.stick_range_x);
    float vy = calcSpeed(diff_y, (float)currentConfig.stick_range_y);

    acc_mouse_x += vx * dt;
    acc_mouse_y -= vy * dt;  // 上下反転（実機確認済み）

    // アキュムレータをオーバーフロー前にクランプ
    acc_mouse_x = constrain(acc_mouse_x, -127.0f, 127.0f);
    acc_mouse_y = constrain(acc_mouse_y, -127.0f, 127.0f);

  } else {
    // --- スクロールモード（Lowerレイヤー）---
    auto calcScrollSpeed = [&](float diff, float range) -> float {
      if (diff == 0.0f) return 0.0f;
      float norm = (fabsf(diff) - dz) / (range - dz);
      norm = constrain(norm, 0.0f, 1.0f);
      return copysignf(norm * norm * currentConfig.scroll_max_speed, diff);
    };

    // Y軸→縦スクロール（上方向が負）
    float sv = calcScrollSpeed(diff_y, (float)currentConfig.stick_range_y);
    float sh = calcScrollSpeed(diff_x, (float)currentConfig.stick_range_x);

    acc_scroll += -sv * dt;  // スティック上 = スクロールアップ
    acc_pan    +=  sh * dt;

    acc_scroll = constrain(acc_scroll, -127.0f, 127.0f);
    acc_pan    = constrain(acc_pan,    -127.0f, 127.0f);

    // マウスアキュムレータはクリア
    acc_mouse_x = 0.0f;
    acc_mouse_y = 0.0f;
  }
}

// =========================================================================
// switchMode
// =========================================================================
void switchMode(OperatingMode newMode) {
  // 現在のBT接続を切断
  if (currentMode != MODE_USB && Bluefruit.connected()) {
    Bluefruit.disconnect(Bluefruit.connHandle());
    delay(100);
  }
  Bluefruit.Advertising.stop();

  currentMode = newMode;
  saveMode(newMode);

  if (newMode == MODE_USB) {
    // USBモード
    analogWrite(BT_LED_PIN, 0);
  } else {
    // BTモード: スロット番号 = newMode - 1
    uint8_t slot = (uint8_t)(newMode - 1);
    startAdvertising(slot);
  }

  lastActivityTime = millis();
  updateLED();
}

// =========================================================================
// BLE初期化
// =========================================================================
void initBLE() {
  Bluefruit.begin();
  Bluefruit.autoConnLed(false);
  Bluefruit.Periph.setConnInterval(6, 6);  // 7.5ms
  Bluefruit.setTxPower(4);
  Bluefruit.setName("PotaKB");
  Bluefruit.Periph.setConnectCallback(onConnect);
  Bluefruit.Periph.setDisconnectCallback(onDisconnect);

  bledis.setManufacturer("0Re0_8192");
  bledis.setModel("PotaKB v2.0");
  bledis.begin();

  blehid.begin();
  bas.begin();

  // Keymap/Config サービス
  keymapSvc.begin();

  keymapChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  keymapChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  keymapChar.setMaxLen(sizeof(current_keymap));
  keymapChar.setWriteCallback(onKeymapWrite);
  keymapChar.begin();
  keymapChar.write((uint8_t*)current_keymap, sizeof(current_keymap));

  configChar.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  configChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  configChar.setMaxLen(sizeof(Config));
  configChar.setWriteCallback(onConfigWrite);
  configChar.begin();
  configChar.write((uint8_t*)&currentConfig, sizeof(Config));
}

// =========================================================================
// startAdvertising: btSlot = 0/1/2
// =========================================================================
void startAdvertising(uint8_t btSlot) {
  // スロットごとにボンド情報を切り替え
  // Bluefruit ライブラリはボンド管理を内部でやるので、
  // スロット切替は一旦全ボンドをクリアせずアドバタイズ開始
  // （ペアリング済なら自動再接続、未ペアリングなら新規ペアリング）
  // ※ 本格的な複数ボンド管理は将来拡張ポイント

  Bluefruit.Advertising.clearData();
  Bluefruit.ScanResponse.clearData();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

// =========================================================================
// BLEコールバック
// =========================================================================
void onConnect(uint16_t conn_handle) {
  lastActivityTime = millis();
  keymap_buf_offset = 0;
  config_buf_offset = 0;
  lastBatteryTier = -1;
  updateBattery();
  // 接続時に現在のキーマップ・設定をBLEキャラクタリスティックに反映
  keymapChar.write((uint8_t*)current_keymap, sizeof(current_keymap));
  configChar.write((uint8_t*)&currentConfig, sizeof(Config));
  updateLED();
}

void onDisconnect(uint16_t conn_handle, uint8_t reason) {
  lastActivityTime = millis();
  updateLED();
}

void onKeymapWrite(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (keymap_buf_offset + len > sizeof(keymap_buf)) keymap_buf_offset = 0;
  memcpy(keymap_buf + keymap_buf_offset, data, len);
  keymap_buf_offset += len;
  if (keymap_buf_offset >= sizeof(keymap_buf)) {
    memcpy(current_keymap, keymap_buf, sizeof(current_keymap));
    saveKeymap();
    keymap_buf_offset = 0;
    // 保存確認: 緑LED一瞬
    analogWrite(LED_GREEN, 0); delay(100); analogWrite(LED_GREEN, 255);
  }
}

void onConfigWrite(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (config_buf_offset + len > sizeof(config_buf)) config_buf_offset = 0;
  memcpy(config_buf + config_buf_offset, data, len);
  config_buf_offset += len;
  if (config_buf_offset >= sizeof(Config)) {
    Config tmp;
    memcpy(&tmp, config_buf, sizeof(Config));
    if (tmp.magic == CONFIG_MAGIC && tmp.version == CONFIG_VERSION) {
      currentConfig = tmp;
      saveConfig();
      config_buf_offset = 0;
      analogWrite(LED_GREEN, 0); delay(100); analogWrite(LED_GREEN, 255);
    } else {
      config_buf_offset = 0;
    }
  }
}

// =========================================================================
// キャリブレーション
// =========================================================================
void doCalibrate() {
  // 青点滅 → 2秒待機 → 平均を中心値に設定
  for (int i = 0; i < 6; i++) {
    analogWrite(LED_BLUE, (i % 2 == 0) ? 0 : 255);
    delay(200);
  }
  analogWrite(LED_BLUE, 255);

  // 64サンプル平均
  long sx = 0, sy = 0;
  for (int i = 0; i < 64; i++) {
    sx += analogRead(STICK_PHYS_Y_PIN);  // A3 → 論理X
    sy += analogRead(STICK_PHYS_X_PIN);  // A2 → 論理Y
    delay(10);
  }
  currentConfig.stick_center_x = (uint16_t)(sx / 64);
  currentConfig.stick_center_y = (uint16_t)(sy / 64);
  smoothed_x = currentConfig.stick_center_x;
  smoothed_y = currentConfig.stick_center_y;
  saveConfig();

  // 完了: 緑LED
  analogWrite(LED_GREEN, 0); delay(300); analogWrite(LED_GREEN, 255);
}

// =========================================================================
// リセット・リブート
// =========================================================================
void doReset() {
  // 紫5回点滅
  for (int i = 0; i < 5; i++) {
    analogWrite(LED_RED, 0); analogWrite(LED_BLUE, 0);
    delay(100);
    analogWrite(LED_RED, 255); analogWrite(LED_BLUE, 255);
    delay(100);
  }
  InternalFS.remove(KEYMAP_FILE);
  InternalFS.remove(CONFIG_FILE);
  delay(100);
  NVIC_SystemReset();
}

void doReboot() {
  delay(100);
  NVIC_SystemReset();
}

// =========================================================================
// isAnyActivity
// =========================================================================
bool isAnyActivity() {
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      if (keyState[r][c]) return true;
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
  Wire.end();
  delay(50);
  NRF_POWER->SYSTEMOFF = 1;
}

// =========================================================================
// LED更新
// =========================================================================
static unsigned long lastBlinkTime = 0;
static bool blinkState = false;

void updateLED() {
  if (isSleeping) return;

  switch (currentMode) {
    case MODE_USB:
      // 緑固定
      analogWrite(LED_RED, 255); analogWrite(LED_GREEN, 0); analogWrite(LED_BLUE, 255);
      analogWrite(BT_LED_PIN, 0);
      break;

    case MODE_BT1: case MODE_BT2: case MODE_BT3:
      // 青（接続中）or 青点滅（アドバタイズ中）
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
// バッテリー更新
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

  // 5%刻みに丸める
  int tier = ((int)pct / 5) * 5;
  if (tier == 0) tier = 1;

  if (tier != lastBatteryTier) {
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
  currentConfig.magic             = CONFIG_MAGIC;
}

void loadConfig() {
  if (!InternalFS.exists(CONFIG_FILE)) return;
  File f = InternalFS.open(CONFIG_FILE, FILE_O_READ);
  if (!f) return;
  Config tmp;
  f.read((uint8_t*)&tmp, sizeof(Config));
  f.close();
  if (tmp.magic == CONFIG_MAGIC && tmp.version == CONFIG_VERSION) {
    currentConfig = tmp;
  }
}

void saveConfig() {
  InternalFS.remove(CONFIG_FILE);
  File f = InternalFS.open(CONFIG_FILE, FILE_O_WRITE);
  if (!f) return;
  f.write((uint8_t*)&currentConfig, sizeof(Config));
  f.close();
}

void loadKeymap() {
  memcpy(current_keymap, default_keymap, sizeof(current_keymap));
  if (!InternalFS.exists(KEYMAP_FILE)) return;
  File f = InternalFS.open(KEYMAP_FILE, FILE_O_READ);
  if (!f) return;
  if (f.size() != sizeof(current_keymap)) {
    f.close();
    InternalFS.remove(KEYMAP_FILE);
    return;
  }
  f.read((uint8_t*)current_keymap, sizeof(current_keymap));
  f.close();

  // 全ゼロなら破損ファイルとみなしてデフォルトに戻す
  bool all_zero = true;
  for (int i = 0; i < LAYOUT_KEY_COUNT && all_zero; i++) {
    if (current_keymap[0][i] != 0) all_zero = false;
  }
  if (all_zero) {
    memcpy(current_keymap, default_keymap, sizeof(current_keymap));
    InternalFS.remove(KEYMAP_FILE);
  }
}

void saveKeymap() {
  InternalFS.remove(KEYMAP_FILE);
  File f = InternalFS.open(KEYMAP_FILE, FILE_O_WRITE);
  if (!f) return;
  f.write((uint8_t*)current_keymap, sizeof(current_keymap));
  f.close();
}

OperatingMode loadMode() {
  if (!InternalFS.exists(MODE_FILE)) return MODE_USB;
  File f = InternalFS.open(MODE_FILE, FILE_O_READ);
  if (!f) return MODE_USB;
  uint8_t m;
  f.read(&m, 1);
  f.close();
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

// =========================================================================
// シリアルコマンド処理
// =========================================================================
void handleSerial() {
  size_t avail = Serial.available();
  if (avail == 0) return;

  if (serialState == SER_RECV_KEYMAP) {
    size_t toRead = min(avail, sizeof(keymap_buf) - (size_t)keymap_buf_offset);
    Serial.readBytes(keymap_buf + keymap_buf_offset, toRead);
    keymap_buf_offset += toRead;
    if (keymap_buf_offset >= sizeof(keymap_buf)) {
      memcpy(current_keymap, keymap_buf, sizeof(current_keymap));
      saveKeymap();
      serialState = SER_IDLE;
      keymap_buf_offset = 0;
      Serial.println("OK");
    }

  } else if (serialState == SER_RECV_CONFIG) {
    size_t toRead = min(avail, sizeof(config_buf) - (size_t)config_buf_offset);
    Serial.readBytes(config_buf + config_buf_offset, toRead);
    config_buf_offset += toRead;
    if (config_buf_offset >= sizeof(config_buf)) {
      Config tmp;
      memcpy(&tmp, config_buf, sizeof(Config));
      if (tmp.magic == CONFIG_MAGIC && tmp.version == CONFIG_VERSION) {
        currentConfig = tmp;
        saveConfig();
        Serial.println("OK");
      } else {
        Serial.println("ERR:INVALID");
      }
      serialState = SER_IDLE;
      config_buf_offset = 0;
    }

  } else {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') {
        serialCmdBuf.trim();
        if      (serialCmdBuf == "READ_KEYMAP")  { Serial.write((uint8_t*)current_keymap, sizeof(current_keymap)); }
        else if (serialCmdBuf == "WRITE_KEYMAP") { serialState = SER_RECV_KEYMAP; keymap_buf_offset = 0; }
        else if (serialCmdBuf == "READ_CONFIG")  { Serial.write((uint8_t*)&currentConfig, sizeof(Config)); }
        else if (serialCmdBuf == "WRITE_CONFIG") { serialState = SER_RECV_CONFIG; config_buf_offset = 0; }
        else if (serialCmdBuf == "GET_BATTERY")  { updateBattery(); Serial.print("BATTERY:"); Serial.println(lastBatteryTier); }
        else if (serialCmdBuf == "CALIBRATE")    { doCalibrate(); Serial.println("OK"); }
        else if (serialCmdBuf == "GET_VERSION")  { Serial.println("PotaKB v2.0"); }
        else if (serialCmdBuf == "READ_STICK")   {
          Serial.print("STICK:");
          Serial.print(analogRead(STICK_PHYS_Y_PIN));  // 論理X
          Serial.print(",");
          Serial.println(analogRead(STICK_PHYS_X_PIN)); // 論理Y
        }
        else if (serialCmdBuf == "CALIB_START")  {
          calibrating = true;
          acc_mouse_x = 0.0f; acc_mouse_y = 0.0f;
          acc_scroll  = 0.0f; acc_pan    = 0.0f;
          lastStickUpdateTime = millis();
          Serial.println("OK");
        }
        else if (serialCmdBuf == "CALIB_END")    {
          calibrating = false;
          lastStickUpdateTime = millis();
          Serial.println("OK");
        }
        serialCmdBuf = "";
      } else {
        serialCmdBuf += c;
        if (serialCmdBuf.length() > 64) serialCmdBuf = "";
      }
    }
  }
}

// -------------------------------------------------------------------------
// --- PotaKB カスタムファームウェア for XIAO nRF52840 ---
// --- バージョン: v2.18 ---
// -------------------------------------------------------------------------
// v2.18 変更点:
// - 電源スイッチのON/OFFの向きをソフトウェアで反転。
//   - HIGH信号でディープスリープに入り、LOW信号でスリープから復帰するように変更。
// v2.17 変更点:
// - Bluetooth切断ロジックを、ユーザー提供のより確実なバージョンに更新。
// -------------------------------------------------------------------------

#include <bluefruit.h>
#include <Wire.h>
#include "Adafruit_MCP23X17.h"
#include <math.h>
#include "Adafruit_TinyUSB.h"

#include "keymap.h"

// --- 動作モード定義 ---
enum OperatingMode_t {
  MODE_BLUETOOTH,
  MODE_USB
};
OperatingMode_t currentMode;

// --- ハードウェアピン定義 ---
#define STICK_X_PIN      A2
#define STICK_Y_PIN      A3
#define BACK_BTN_PIN     D6
#define NEXT_BTN_PIN     D10
#define POWER_SW_PIN     D7
#define POWER_LED_PIN    D0
#define BLUETOOTH_LED_PIN D1
#define BATTERY_PIN      A0

// --- LED制御設定 ---
#define LED_BRIGHTNESS   50
#define BLINK_INTERVAL_MS 1000

// --- バッテリー測定設定 ---
#define VBAT_MV_PER_LSB (3.3 * 1000 / 1023)
#define VBAT_DIVIDER_R1 1000000.0f
#define VBAT_DIVIDER_R2 510000.0f
#define REAL_VBAT_MV_PER_LSB (VBAT_MV_PER_LSB * (VBAT_DIVIDER_R1 + VBAT_DIVIDER_R2) / VBAT_DIVIDER_R2)
#define BATT_MAX_MV 4200
#define BATT_MIN_MV 3000
#define BATT_CHECK_IDLE_MS 5000 

// --- HIDサービスの準備 ---
BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas bas;
Adafruit_USBD_HID usb_hid;

uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
  TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2))
};

// --- I/Oエキスパンダの準備 ---
Adafruit_MCP23X17 mcp;

// --- 物理配線(Row/Col)に準拠したキーマップ ---
uint16_t keymap[NUM_LAYERS][ROWS][COLS] = {0};

// --- 状態管理用のグローバル変数 ---
bool keyState[ROWS][COLS] = {0};
bool backBtnState = false, nextBtnState = false;

uint8_t prev_kb_modifiers = 0;
uint8_t prev_kb_keycodes[6] = {0};
uint8_t prev_mouse_buttons = 0;

uint8_t active_layer = 0;
const unsigned long sleepTimeout = 300000;
unsigned long lastActivityTime = 0;
bool isSleeping = false;
const unsigned long debounceDelay = 5;
bool lastRawKeyState[ROWS][COLS] = {0};
unsigned long keyChangeTime[ROWS][COLS] = {0};
bool lastRawBackBtnState = false, lastRawNextBtnState = false;
unsigned long backBtnChangeTime = 0, nextBtnChangeTime = 0;
int xReadings[SMOOTHING_SAMPLES], yReadings[SMOOTHING_SAMPLES];
int readIndex = 0;
long xTotal = 0, yTotal = 0;
float mouseX_accumulator = 0;
float mouseY_accumulator = 0;
bool modeSwitchKeyWasPressed = false;


// =========================================================================
// === 関数プロトタイプ宣言 ===
// =========================================================================
void initBluetooth();
bool initMCP23017();
void scanMatrix();
void scanDirectKeys();
bool processAllInputs();
bool isAnyActivity();
void goToSystemOnSleep();
void wakeUpFromSystemOnSleep();
void goToSystemOffSleep();
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void updateBatteryLevel();
void switchMode(OperatingMode_t newMode);
void sendKeyboardReport(uint8_t modifiers, uint8_t keycodes[6]);
void sendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t scroll, int8_t pan);
void setXiaoLed(int r, int g, int b);


// =========================================================================
// === 初期化処理 (setup) ===
// =========================================================================
void setup() {
  pinMode(LED_BUILTIN, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  
  pinMode(POWER_LED_PIN, OUTPUT);
  analogWrite(POWER_LED_PIN, LED_BRIGHTNESS);
  
  pinMode(BLUETOOTH_LED_PIN, OUTPUT);
  analogWrite(BLUETOOTH_LED_PIN, 0);
  
  pinMode(BACK_BTN_PIN, INPUT_PULLUP);
  pinMode(NEXT_BTN_PIN, INPUT_PULLUP);
  
  // ★★★ 修正点1: 電源スイッチのウェイクアップ設定を反転 ★★★
  pinMode(POWER_SW_PIN, INPUT_PULLUP);
  // HIGHでスリープ、LOWでウェイクアップするように設定
  nrf_gpio_cfg_sense_input(digitalPinToPinName(POWER_SW_PIN), NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setBootProtocol(HID_ITF_PROTOCOL_KEYBOARD);
  usb_hid.begin();
  
  const uint8_t sw_to_matrix[64][2] = {
    {0,0}, {0,1}, {0,2}, {0,3}, {1,1}, {1,2}, {1,3}, {0,4}, {2,1}, {2,2}, {2,3}, {1,4}, {3,1}, {3,2}, {3,3}, {2,4},
    {4,1}, {4,2}, {4,3}, {3,4}, {5,1}, {5,2}, {5,3}, {4,4}, {6,1}, {6,2}, {6,3}, {5,4}, {4,5}, {7,1}, {7,2}, {7,3},
    {6,4}, {5,5}, {1,0}, {1,5}, {3,6}, {7,4}, {6,5}, {2,0}, {2,5}, {0,7}, {4,7}, {4,6}, {3,0}, {3,5}, {1,7}, {5,7},
    {5,6}, {4,0}, {0,6}, {2,7}, {6,7}, {6,6}, {5,0}, {1,6}, {7,5}, {7,7}, {6,0}, {1,6}, {3,7}, {7,0}, {5,0}
  };
  const uint8_t physical_layout_order[] = {
    1, 62, 2, 5, 9, 13, 17, 21, 25, 30, 35, 40, 45, 50, 55, 59, 63,
    3, 6, 10, 14, 18, 22, 26, 31, 36, 41, 46, 51, 56, 60,
    4, 7, 11, 15, 19, 23, 27, 32, 37, 42, 47, 52, 57, 61,
    8, 12, 16, 20, 24, 28, 33, 38, 43, 48, 53, 58,
    29, 34, 39, 44, 49, 54
  };
  for (int l = 0; l < NUM_LAYERS; l++) {
    for (int i = 0; i < (LAYOUT_KEY_COUNT - 2); i++) {
      int sw = physical_layout_order[i];
      int r = sw_to_matrix[sw - 1][0];
      int c = sw_to_matrix[sw - 1][1];
      if (i < (sizeof(layout[0])/sizeof(uint16_t))) keymap[l][r][c] = layout[l][i];
    }
  }
  
  if (!initMCP23017()) {
    while (1) {}
  }
  
  initBluetooth();
  updateBatteryLevel();
  for (int i = 0; i < SMOOTHING_SAMPLES; i++) { xReadings[i] = 0; yReadings[i] = 0; }

  unsigned long start_time = millis();
  while (!TinyUSBDevice.mounted() && (millis() - start_time < 2000)) {
    delay(10);
  }
  
  if (TinyUSBDevice.mounted()) {
    switchMode(MODE_USB);
  } else {
    switchMode(MODE_BLUETOOTH);
  }
  
  lastActivityTime = millis();
}

// =========================================================================
// === メインループ (loop) ===
// =========================================================================
void loop() {
  // ★★★ 修正点2: スリープに入る条件を反転 ★★★
  if (digitalRead(POWER_SW_PIN) == HIGH) goToSystemOffSleep();

  if (currentMode == MODE_USB && !TinyUSBDevice.mounted()) {
    switchMode(MODE_BLUETOOTH);
  }

  scanMatrix();
  scanDirectKeys();
  bool activity = processAllInputs();

  if (currentMode == MODE_BLUETOOTH) {
    static bool batteryCheckedSinceActivity = false;
    if (activity) {
      lastActivityTime = millis();
      batteryCheckedSinceActivity = false;
    }
    
    if (Bluefruit.connected() && !isSleeping) {
      if ((millis() - lastActivityTime > BATT_CHECK_IDLE_MS) && !batteryCheckedSinceActivity) {
        updateBatteryLevel();
        batteryCheckedSinceActivity = true;
      }
    }
    
    if (isSleeping) {
      setXiaoLed(0, 0, 0);
      analogWrite(BLUETOOTH_LED_PIN, 0);
      if (isAnyActivity()) wakeUpFromSystemOnSleep();
      waitForEvent(); 
    } else {
      setXiaoLed(0, 0, LED_BRIGHTNESS);
      
      if (!Bluefruit.connected()) {
        static unsigned long lastBlinkTime = 0;
        if (millis() - lastBlinkTime > BLINK_INTERVAL_MS) {
          lastBlinkTime = millis();
          static bool ledState = false;
          ledState = !ledState;
          if (ledState) {
            analogWrite(BLUETOOTH_LED_PIN, LED_BRIGHTNESS);
          } else {
            analogWrite(BLUETOOTH_LED_PIN, 0);
          }
        }
      } else {
        analogWrite(BLUETOOTH_LED_PIN, LED_BRIGHTNESS);
      }
      if (millis() - lastActivityTime > sleepTimeout) goToSystemOnSleep();
    }
  } else {
    setXiaoLed(0, LED_BRIGHTNESS, 0);
    analogWrite(BLUETOOTH_LED_PIN, 0);
  }
}

// =========================================================================
// === 各種機能の関数 ===
// =========================================================================

void switchMode(OperatingMode_t newMode) {
  currentMode = newMode;
  if (newMode == MODE_USB) {
    if (Bluefruit.connected()) {
      uint16_t conn_handle = Bluefruit.connHandle();
      Bluefruit.disconnect(conn_handle);
      delay(100);
    }
    Bluefruit.Advertising.stop();
    
    setXiaoLed(0, LED_BRIGHTNESS, 0);
    analogWrite(BLUETOOTH_LED_PIN, 0);
  } else {
    Bluefruit.Advertising.start(0);
    setXiaoLed(0, 0, LED_BRIGHTNESS);
  }
  lastActivityTime = millis();
}

void setXiaoLed(int r, int g, int b) {
  analogWrite(LED_RED, 255 - r);
  analogWrite(LED_GREEN, 255 - g);
  analogWrite(LED_BLUE, 255 - b);
}

void sendKeyboardReport(uint8_t modifiers, uint8_t keycodes[6]) {
  if (currentMode == MODE_USB) {
    if (TinyUSBDevice.mounted() && usb_hid.ready()) {
      usb_hid.keyboardReport(1, modifiers, keycodes);
    }
  } else {
    if (Bluefruit.connected()) {
      blehid.keyboardReport(modifiers, keycodes);
    }
  }
}

void sendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t scroll, int8_t pan) {
  if (currentMode == MODE_USB) {
    if (TinyUSBDevice.mounted() && usb_hid.ready()) {
      usb_hid.mouseReport(2, buttons, x, y, scroll, pan);
    }
  } else {
    if (Bluefruit.connected()) {
      blehid.mouseReport(buttons, x, y, scroll, pan);
    }
  }
}

void initBluetooth() {
  Bluefruit.begin();
  Bluefruit.autoConnLed(false);
  Bluefruit.Periph.setConnInterval(6, 12);
  Bluefruit.setTxPower(4);
  Bluefruit.setName("PotaKB");
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
  
  bledis.setManufacturer("Custom Keyboards");
  bledis.setModel("PotaKB v2.18");
  bledis.begin();
  
  blehid.begin();
  bas.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.Advertising.addService(bas);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
}

bool initMCP23017() {
  if (!mcp.begin_I2C()) return false;
  for (int i = 0; i < 8; i++)  mcp.pinMode(i, INPUT_PULLUP);
  for (int i = 8; i < 16; i++) mcp.pinMode(i, OUTPUT);
  return true;
}

void scanMatrix() {
  unsigned long currentTime = millis();
  for (int c = 0; c < COLS; c++) {
    mcp.digitalWrite(c + 8, LOW);
    delayMicroseconds(10);
    for (int r = 0; r < ROWS; r++) {
      bool rawState = (mcp.digitalRead(r) == LOW);
      if (rawState != lastRawKeyState[r][c]) keyChangeTime[r][c] = currentTime;
      lastRawKeyState[r][c] = rawState;
      if ((currentTime - keyChangeTime[r][c]) > debounceDelay) keyState[r][c] = rawState;
    }
    mcp.digitalWrite(c + 8, HIGH);
  }
}

void scanDirectKeys() {
  unsigned long currentTime = millis();
  bool rawBackState = (digitalRead(BACK_BTN_PIN) == LOW);
  if (rawBackState != lastRawBackBtnState) backBtnChangeTime = currentTime;
  lastRawBackBtnState = rawBackState;
  if ((currentTime - backBtnChangeTime) > debounceDelay) backBtnState = rawBackState;
  bool rawNextState = (digitalRead(NEXT_BTN_PIN) == LOW);
  if (rawNextState != lastRawNextBtnState) nextBtnChangeTime = currentTime;
  lastRawNextBtnState = rawNextState;
  if ((currentTime - nextBtnChangeTime) > debounceDelay) nextBtnState = rawNextState;
}

bool processAllInputs() {
  uint8_t kb_modifiers = 0;
  uint8_t kb_keycodes[6] = {0};
  uint8_t kb_keycount = 0;
  
  uint8_t mouse_buttons = 0;
  int8_t mouse_scroll = 0, mouse_pan = 0;

  bool modeSwitchPressedNow = false;
  bool activity = false;

  active_layer = 0;
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (keyState[r][c] && keymap[0][r][c] == L_LOWER) {
        active_layer = 1;
        break;
      }
    }
    if(active_layer > 0) break;
  }

  auto process_a_key = [&](uint16_t key) {
    switch (key) {
      case KC_NO:
      case L_LOWER:
        break;

      case SW_USB_BT:
        modeSwitchPressedNow = true;
        break;

      case KC_MS_UP:    mouseY_accumulator -= MOUSE_MOVE_SPEED; break;
      case KC_MS_DOWN:  mouseY_accumulator += MOUSE_MOVE_SPEED; break;
      case KC_MS_LEFT:  mouseX_accumulator -= MOUSE_MOVE_SPEED; break;
      case KC_MS_RIGHT: mouseX_accumulator += MOUSE_MOVE_SPEED; break;
      
      case KC_MS_BTN1:  mouse_buttons |= MOUSE_BUTTON_LEFT;   break;
      case KC_MS_BTN2:  mouse_buttons |= MOUSE_BUTTON_RIGHT;  break;
      case KC_MS_BTN3:  mouse_buttons |= MOUSE_BUTTON_MIDDLE; break;
      case KC_MS_BTN4:  mouse_buttons |= MOUSE_BUTTON_BACKWARD; break;
      case KC_MS_BTN5:  mouse_buttons |= MOUSE_BUTTON_FORWARD;  break;
      
      default:
        if (key >= HID_KEY_CONTROL_LEFT && key <= HID_KEY_GUI_RIGHT) {
          kb_modifiers |= (1 << (key - HID_KEY_CONTROL_LEFT));
        } else if (kb_keycount < 6) {
          kb_keycodes[kb_keycount++] = key;
        }
        break;
    }
  };

  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (keyState[r][c]) {
        uint16_t key = keymap[active_layer][r][c];
        if (key == KC_TRNS && active_layer > 0) key = keymap[active_layer - 1][r][c];
        process_a_key(key);
      }
    }
  }
  if (nextBtnState) {
    uint16_t key = layout[active_layer][63];
    if (key == KC_TRNS && active_layer > 0) key = layout[active_layer - 1][63];
    process_a_key(key);
  }
  if (backBtnState) {
    uint16_t key = layout[active_layer][64];
    if (key == KC_TRNS && active_layer > 0) key = layout[active_layer - 1][64];
    process_a_key(key);
  }

  if (modeSwitchPressedNow && !modeSwitchKeyWasPressed) {
    if (currentMode == MODE_BLUETOOTH) {
      if (TinyUSBDevice.mounted()) { switchMode(MODE_USB); }
    } else {
      switchMode(MODE_BLUETOOTH);
    }
  }
  modeSwitchKeyWasPressed = modeSwitchPressedNow;

  static unsigned long lastMoveTime = 0;
  if (millis() - lastMoveTime >= 8) {
    lastMoveTime = millis();
    xTotal -= xReadings[readIndex]; yTotal -= yReadings[readIndex];
    xReadings[readIndex] = analogRead(STICK_Y_PIN); yReadings[readIndex] = analogRead(STICK_X_PIN);
    xTotal += xReadings[readIndex]; yTotal += yReadings[readIndex];
    readIndex = (readIndex + 1) % SMOOTHING_SAMPLES;
    int xAvg = xTotal / SMOOTHING_SAMPLES; int yAvg = yTotal / SMOOTHING_SAMPLES;
    int xDiff = xAvg - STICK_CENTER_X; int yDiff = STICK_CENTER_Y - yAvg;
    
    if (active_layer == 1) {
      if (abs(yDiff) > STICK_DEADZONE) mouse_scroll = (yDiff > 0) ? -1 : 1;
      if (abs(xDiff) > STICK_DEADZONE) mouse_pan = (xDiff > 0) ? 1 : -1;
      mouseX_accumulator = 0; mouseY_accumulator = 0;
    } else {
      float speed_magnitude = max(abs(xDiff), abs(yDiff));
      if (speed_magnitude > STICK_DEADZONE) {
        float direction_magnitude = sqrt(xDiff * xDiff + yDiff * yDiff);
        if (direction_magnitude < 1) direction_magnitude = 1;
        float dirX = xDiff / direction_magnitude; float dirY = yDiff / direction_magnitude;
        float speed;
        if (speed_magnitude < MOUSE_ACCEL_THRESHOLD) {
          speed = map(speed_magnitude, STICK_DEADZONE, MOUSE_ACCEL_THRESHOLD, 0, MOUSE_LOW_SPEED);
        } else {
          float maxMagnitude = (STICK_CENTER_X);
          float range = maxMagnitude - MOUSE_ACCEL_THRESHOLD;
          float progress = speed_magnitude - MOUSE_ACCEL_THRESHOLD;
          speed = MOUSE_LOW_SPEED + (progress / range) * (MOUSE_HIGH_SPEED - MOUSE_LOW_SPEED);
        }
        mouseX_accumulator += dirX * speed;
        mouseY_accumulator += dirY * speed;
      } else {
        mouseX_accumulator = 0;
        mouseY_accumulator = 0;
      }
    }
  }
  int8_t mouseX_to_send = (int8_t)trunc(mouseX_accumulator);
  int8_t mouseY_to_send = (int8_t)trunc(mouseY_accumulator);
  mouseX_accumulator -= mouseX_to_send;
  mouseY_accumulator -= mouseY_to_send;
  
  if (kb_modifiers != prev_kb_modifiers || memcmp(kb_keycodes, prev_kb_keycodes, 6) != 0) {
    sendKeyboardReport(kb_modifiers, kb_keycodes);
    prev_kb_modifiers = kb_modifiers;
    memcpy(prev_kb_keycodes, kb_keycodes, 6);
    activity = true;
  }

  if (mouseX_to_send != 0 || mouseY_to_send != 0 || mouse_buttons != prev_mouse_buttons || 
      mouse_scroll != 0 || mouse_pan != 0) {
    sendMouseReport(mouse_buttons, mouseX_to_send, mouseY_to_send, mouse_scroll, mouse_pan);
    prev_mouse_buttons = mouse_buttons;
    activity = true;
  }
  
  return activity;
}

bool isAnyActivity() {
  for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) if (keyState[r][c]) return true;
  if (backBtnState || nextBtnState) return true;
  int xVal = analogRead(STICK_Y_PIN);
  int yVal = analogRead(STICK_X_PIN);
  if (abs(xVal - STICK_CENTER_X) > STICK_DEADZONE || abs(yVal - STICK_CENTER_Y) > STICK_DEADZONE) return true;
  return false;
}

void goToSystemOnSleep() {
  isSleeping = true;
}

void wakeUpFromSystemOnSleep() {
  isSleeping = false;
  lastActivityTime = millis();
}

void goToSystemOffSleep() {
  setXiaoLed(0,0,0);
  analogWrite(BLUETOOTH_LED_PIN, 0);
  analogWrite(POWER_LED_PIN, 0);
  Wire.end();
  NRF_POWER->SYSTEMOFF = 1;
}

void connect_callback(uint16_t conn_handle) {
  lastActivityTime = millis();
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  lastActivityTime = millis();
}

void updateBatteryLevel() {
  const int numReadings = 10;
  long total = 0;
  for (int i = 0; i < numReadings; i++) {
    total += analogRead(BATTERY_PIN);
    delay(1);
  }
  int adc_avg = total / numReadings;
  int battery_mv = (int)(adc_avg * REAL_VBAT_MV_PER_LSB);
  uint8_t battery_percent = map(battery_mv, BATT_MIN_MV, BATT_MAX_MV, 0, 100);
  battery_percent = constrain(battery_percent, 0, 100);
  int rounded_percent = round((float)battery_percent / 30.0) * 30;
  static int last_sent_percent = -1;
  if (rounded_percent != last_sent_percent) {
    bas.notify(rounded_percent);
    last_sent_percent = rounded_percent;
  }
}
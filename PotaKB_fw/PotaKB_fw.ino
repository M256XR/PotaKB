// -------------------------------------------------------------------------
// --- PotaKB for XIAO nRF52840 ---
// ---------- Ver: 1.3.5 (Compile Fix) ----------
// -------------------------------------------------------------------------

#include <bluefruit.h>
#include <Wire.h>
#include "Adafruit_MCP23X17.h"
#include <math.h>
#include "Adafruit_TinyUSB.h"
#include <InternalFileSystem.h> 
#include "keymap.h"

using namespace Adafruit_LittleFS_Namespace;

const char* KEYMAP_FILENAME = "keymap.bin";
const char* CONFIG_FILENAME = "config.bin";
const char* BOOT_FLAG_FILENAME = "boot.flag";

#define KEYMAP_SERVICE_UUID   "adaf0001-c332-42a8-93bd-25e905756cb8"
#define KEYMAP_CHAR_UUID      "adaf0002-c332-42a8-93bd-25e905756cb8"
#define CONFIG_CHAR_UUID      "adaf0003-c332-42a8-93bd-25e905756cb8"
BLEService keymapService(KEYMAP_SERVICE_UUID);
BLECharacteristic keymapChar(KEYMAP_CHAR_UUID);
BLECharacteristic configChar(CONFIG_CHAR_UUID);

#pragma pack(push, 1)
struct Config {
  uint16_t stick_center_x;
  uint16_t stick_center_y;
  uint16_t stick_deadzone;
  float mouse_high_speed;
  float mouse_low_speed;
  uint16_t mouse_accel_threshold;
  uint16_t smoothing_samples;
  uint16_t scroll_min_threshold;
  uint16_t scroll_max_threshold;
  float scroll_min_speed;
  float scroll_max_speed;
  uint16_t scroll_interval_ms;
  float mouse_high_speed_bt;
  float scroll_min_speed_bt;
  float scroll_max_speed_bt;
  uint32_t sleep_timeout;
  uint16_t led_brightness;
  uint16_t blink_interval_ms;
  uint32_t magic;
};
#pragma pack(pop)

Config currentConfig;
float active_mouse_high_speed;
float active_scroll_min_speed;
float active_scroll_max_speed;
enum OperatingMode_t { MODE_BLUETOOTH, MODE_USB };
OperatingMode_t currentMode;
enum SerialState_t { STATE_IDLE, STATE_RECEIVING_KEYMAP, STATE_RECEIVING_CONFIG };
SerialState_t serialState = STATE_IDLE;

#define STICK_X_PIN      A2
#define STICK_Y_PIN      A3
#define BACK_BTN_PIN     D6
#define NEXT_BTN_PIN     D10
#define POWER_SW_PIN     D7
#define POWER_LED_PIN    D0
#define BLUETOOTH_LED_PIN D1

#define BATTERY_CHECK_INTERVAL_MS 60000UL
unsigned long lastBatteryCheckTime = 0;
int lastNotifiedBatteryTier = -1;

BLEDis bledis;
BLEHidAdafruit blehid;
BLEBas bas;
Adafruit_USBD_HID usb_hid;
uint8_t const desc_hid_report[] = { TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)), TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2)) };
Adafruit_MCP23X17 mcp;

uint16_t current_keymap[NUM_LAYERS][LAYOUT_KEY_COUNT];
int matrix_to_layout_index[ROWS][COLS];
uint8_t keymap_buffer[sizeof(current_keymap)];
uint16_t keymap_buffer_offset = 0;
uint8_t config_buffer[sizeof(Config)];
uint16_t config_buffer_offset = 0;

bool keyState[ROWS][COLS] = {0};
bool backBtnState = false, nextBtnState = false;
uint8_t prev_kb_modifiers = 0;
uint8_t prev_kb_keycodes[6] = {0};
uint8_t prev_mouse_buttons = 0;
uint8_t active_layer = 0;
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
float scroll_accumulator = 0;
float pan_accumulator = 0;
String serialCmdBuffer = "";

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
void keymap_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
void config_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
void loadDefaultConfig();
void loadConfigFromFlash();
void saveConfigToFlash();
void applyConfig();
void applyModeConfig(OperatingMode_t mode);
void handleSerialCommands();

void setup() {
  TinyUSBDevice.setID(0x239A, 0x802A);
  TinyUSBDevice.setManufacturerDescriptor("0Re0_8192");
  TinyUSBDevice.setProductDescriptor("PotaKB");          // ★ここ！

  Serial.begin(115200);
  
  InternalFS.begin(); 
  loadDefaultConfig();
  loadConfigFromFlash();
  
  pinMode(LED_BUILTIN, INPUT);
  pinMode(LED_RED, OUTPUT); pinMode(LED_GREEN, OUTPUT); pinMode(LED_BLUE, OUTPUT);
  setXiaoLed(0,0,0);
  pinMode(POWER_LED_PIN, OUTPUT);
  pinMode(BLUETOOTH_LED_PIN, OUTPUT); analogWrite(BLUETOOTH_LED_PIN, 0);
  pinMode(BACK_BTN_PIN, INPUT_PULLUP); pinMode(NEXT_BTN_PIN, INPUT_PULLUP);
  pinMode(POWER_SW_PIN, INPUT_PULLUP);
  nrf_gpio_cfg_sense_input(digitalPinToPinName(POWER_SW_PIN), NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

  usb_hid.setPollInterval(1);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();
  
  if (InternalFS.exists(BOOT_FLAG_FILENAME)) {
    InternalFS.remove(BOOT_FLAG_FILENAME);
    memcpy(current_keymap, layout, sizeof(current_keymap));
    setXiaoLed(0, 0, 255); delay(1000); setXiaoLed(0,0,0);
  } else {
    memcpy(current_keymap, layout, sizeof(current_keymap)); 
    if (InternalFS.exists(KEYMAP_FILENAME)) {
      File keymapFile = InternalFS.open(KEYMAP_FILENAME, FILE_O_READ);
      if (keymapFile) {
        keymapFile.read((uint8_t*)current_keymap, sizeof(current_keymap));
        keymapFile.close();
      }
    }
  }

  initMCP23017();
  Wire.setClock(400000);
  for(int r=0; r<ROWS; r++) { for(int c=0; c<COLS; c++) { matrix_to_layout_index[r][c] = -1; } }
  const uint8_t sw_to_matrix[64][2] = { {0,0}, {0,1}, {0,2}, {0,3}, {1,1}, {1,2}, {1,3}, {0,4}, {2,1}, {2,2}, {2,3}, {1,4}, {3,1}, {3,2}, {3,3}, {2,4}, {4,1}, {4,2}, {4,3}, {3,4}, {5,1}, {5,2}, {5,3}, {4,4}, {6,1}, {6,2}, {6,3}, {5,4}, {4,5}, {7,1}, {7,2}, {7,3}, {6,4}, {5,5}, {1,0}, {1,5}, {3,6}, {7,4}, {6,5}, {2,0}, {2,5}, {0,7}, {4,7}, {4,6}, {3,0}, {3,5}, {1,7}, {5,7}, {5,6}, {4,0}, {0,6}, {2,7}, {6,7}, {6,6}, {5,0}, {1,6}, {7,5}, {7,7}, {6,0}, {2,6}, {3,7}, {7,0}, {0,5}, {0,0} };
  const uint8_t physical_layout_order[] = { 1, 62, 2, 5, 9, 13, 17, 21, 25, 30, 35, 40, 45, 50, 55, 59, 63, 3, 6, 10, 14, 18, 22, 26, 31, 36, 41, 46, 51, 56, 60, 4, 7, 11, 15, 19, 23, 27, 32, 37, 42, 47, 52, 57, 61, 8, 12, 16, 20, 24, 28, 33, 38, 43, 48, 53, 58, 29, 34, 39, 44, 49, 54 };
  for (int i = 0; i < (LAYOUT_KEY_COUNT - 2); i++) { int sw = physical_layout_order[i]; int r = sw_to_matrix[sw - 1][0]; int c = sw_to_matrix[sw - 1][1]; matrix_to_layout_index[r][c] = i; }
  
  initBluetooth(); 
  updateBatteryLevel();
  for (int i = 0; i < SMOOTHING_SAMPLES; i++) { xReadings[i] = 0; yReadings[i] = 0; }
  
  unsigned long start_time = millis();
  while (!TinyUSBDevice.mounted() && (millis() - start_time < 2000)) { delay(10); }
  if (TinyUSBDevice.mounted()) {
    switchMode(MODE_USB);
  } else {
    switchMode(MODE_BLUETOOTH);
  }
  
  applyConfig();
  lastActivityTime = millis();
}

void loop() {
  if (digitalRead(POWER_SW_PIN) == HIGH) goToSystemOffSleep();
  if (currentMode == MODE_USB && !TinyUSBDevice.mounted()) { switchMode(MODE_BLUETOOTH); }

  if (Serial.available()) {
    handleSerialCommands();
  }

  scanMatrix(); 
  scanDirectKeys();
  bool activity = processAllInputs();

  if (currentMode == MODE_BLUETOOTH) {
    if (activity) { lastActivityTime = millis(); }
    if (Bluefruit.connected() && (millis() - lastBatteryCheckTime > BATTERY_CHECK_INTERVAL_MS)) {
      updateBatteryLevel();
    }
    
    if (isSleeping) { 
      if (isAnyActivity()) wakeUpFromSystemOnSleep(); 
      waitForEvent(); 
    } else { 
      if (!Bluefruit.connected()) { 
        static unsigned long lastBlinkTime = 0; 
        if (millis() - lastBlinkTime > currentConfig.blink_interval_ms) { 
          lastBlinkTime = millis(); 
          static bool ledState = false; 
          ledState = !ledState; 
          analogWrite(BLUETOOTH_LED_PIN, ledState ? currentConfig.led_brightness : 0); 
        } 
      }
      if (millis() - lastActivityTime > currentConfig.sleep_timeout) goToSystemOnSleep(); 
    }
  }
}

void handleSerialCommands() {
  size_t len = Serial.available();
  if (len == 0) return;

  if (serialState == STATE_RECEIVING_KEYMAP) {
    size_t bytesToRead = min(len, sizeof(keymap_buffer) - keymap_buffer_offset);
    Serial.readBytes(keymap_buffer + keymap_buffer_offset, bytesToRead);
    keymap_buffer_offset += bytesToRead;
    if (keymap_buffer_offset >= sizeof(keymap_buffer)) {
      InternalFS.remove(KEYMAP_FILENAME);
      File keymapFile = InternalFS.open(KEYMAP_FILENAME, FILE_O_WRITE);
      if (keymapFile) {
        keymapFile.write(keymap_buffer, sizeof(keymap_buffer));
        keymapFile.close();
        Serial.println("OK");  // 書き込み成功を通知
        Serial.flush();
      } else {
        Serial.println("ERROR");  // エラーを通知
        Serial.flush();
      }
      serialState = STATE_IDLE;
      keymap_buffer_offset = 0;
    }
  } else if (serialState == STATE_RECEIVING_CONFIG) {
    size_t bytesToRead = min(len, sizeof(config_buffer) - config_buffer_offset);
    Serial.readBytes(config_buffer + config_buffer_offset, bytesToRead);
    config_buffer_offset += bytesToRead;
    if (config_buffer_offset >= sizeof(config_buffer)) {
      memcpy(&currentConfig, config_buffer, sizeof(Config));
      if (currentConfig.magic == 0x504F5441) {
        saveConfigToFlash();
        applyConfig();
        Serial.println("OK");  // 書き込み成功を通知
        Serial.flush();
      } else {
        Serial.println("ERROR");  // マジックナンバー不正
        Serial.flush();
      }
      serialState = STATE_IDLE;
      config_buffer_offset = 0;
    }
  } else {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') {
        if (serialCmdBuffer.startsWith("READ_KEYMAP")) {
          Serial.write((uint8_t*)current_keymap, sizeof(current_keymap));
          Serial.flush();
        }
        else if (serialCmdBuffer.startsWith("WRITE_KEYMAP")) { 
          serialState = STATE_RECEIVING_KEYMAP; 
          keymap_buffer_offset = 0; 
        }
        else if (serialCmdBuffer.startsWith("READ_CONFIG")) {
          Serial.write((uint8_t*)&currentConfig, sizeof(Config));
          Serial.flush();
        }
        else if (serialCmdBuffer.startsWith("WRITE_CONFIG")) { 
          serialState = STATE_RECEIVING_CONFIG; 
          config_buffer_offset = 0; 
        }
        else if (serialCmdBuffer.startsWith("GET_BATTERY")) { 
          updateBatteryLevel(); 
          Serial.print("BATTERY:"); 
          Serial.println(lastNotifiedBatteryTier);
          Serial.flush();
        }
        else if (serialCmdBuffer.startsWith("REBOOT")) { 
          Serial.println("OK");  // 再起動を確認
          Serial.flush();
          delay(100);  // シリアル送信完了を待つ
          NVIC_SystemReset(); 
        }
        serialCmdBuffer = "";
      } else {
        serialCmdBuffer += c;
        if (serialCmdBuffer.length() > 64) serialCmdBuffer = "";
      }
    }
  }
}

void updateBatteryLevel() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  delay(5);
  long total = 0;
  for (int i = 0; i < 10; i++) { total += analogRead(PIN_VBAT); delay(1); }
  pinMode(VBAT_ENABLE, INPUT);
  
  float volt = (float)(total / 10) / 1024.0 * 3.6 * (1510.0 / 510.0);
  uint8_t percent = 100;
  if (volt <= 3.5) percent = 1; 
  else if (volt <= 3.62) percent = 3; 
  else if (volt <= 3.8) percent = (uint8_t)(((volt - 3.62) * 277.78 + 5) / 5 + 0.5) * 5; 
  else if (volt <= 4.1) percent = (uint8_t)(((volt - 3.8) * 150 + 55) / 5 + 0.5) * 5; 
  
  if (percent != lastNotifiedBatteryTier) {
    bas.notify(percent);
    lastNotifiedBatteryTier = percent;
  }
}

void keymap_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
    if (keymap_buffer_offset + len > sizeof(keymap_buffer)) { keymap_buffer_offset = 0; }
    memcpy(keymap_buffer + keymap_buffer_offset, data, len);
    keymap_buffer_offset += len;

    if (keymap_buffer_offset >= sizeof(keymap_buffer)) {
        keymap_buffer_offset = 0; 
        InternalFS.remove(KEYMAP_FILENAME); 
        File keymapFile = InternalFS.open(KEYMAP_FILENAME, FILE_O_WRITE);
        if (keymapFile) { 
            keymapFile.write(keymap_buffer, sizeof(keymap_buffer)); 
            keymapFile.close(); 
        }
    }
}

void config_write_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  if (config_buffer_offset + len > sizeof(config_buffer)) { config_buffer_offset = 0; }
  memcpy(config_buffer + config_buffer_offset, data, len);
  config_buffer_offset += len;

  if (config_buffer_offset >= sizeof(Config)) {
    Config tempConfig;
    memcpy(&tempConfig, config_buffer, sizeof(Config));
    if (tempConfig.magic == 0x504F5441) {
      memcpy(&currentConfig, &tempConfig, sizeof(Config));
      saveConfigToFlash();
      applyConfig();
      config_buffer_offset = 0;
      setXiaoLed(0, 255, 0); delay(200); setXiaoLed(0, 0, 0);
      delay(500);
      NVIC_SystemReset();
    } else {
      config_buffer_offset = 0;
      setXiaoLed(255, 0, 0); delay(200); setXiaoLed(0, 0, 0);
    }
  }
}

void goToSystemOffSleep() {
  setXiaoLed(0,0,0);
  analogWrite(BLUETOOTH_LED_PIN, 0);
  analogWrite(POWER_LED_PIN, 0);
  Wire.end();
  delay(50); 
  NRF_POWER->SYSTEMOFF = 1;
}

void connect_callback(uint16_t conn_handle) { 
  lastActivityTime = millis(); 
  keymap_buffer_offset = 0; 
  config_buffer_offset = 0;
  lastBatteryCheckTime = millis();
  lastNotifiedBatteryTier = -1;
  updateBatteryLevel();
  keymapChar.write((uint8_t*)current_keymap, sizeof(current_keymap));
  configChar.write((uint8_t*)&currentConfig, sizeof(Config));
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) { lastActivityTime = millis(); }

void initBluetooth() { 
  Bluefruit.begin(); 
  Bluefruit.autoConnLed(false); 
  Bluefruit.Periph.setConnInterval(6, 6);
  Bluefruit.setTxPower(4); 
  Bluefruit.setName("PotaKB"); 
  Bluefruit.Periph.setConnectCallback(connect_callback); 
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback); 
  bledis.setManufacturer("0Re0_8192"); 
  bledis.setModel("PotaKB v1.3.5"); 
  bledis.begin(); 
  blehid.begin(); 
  bas.begin(); 
  keymapService.begin(); 
  
  keymapChar.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP | CHR_PROPS_READ); 
  keymapChar.setPermission(SECMODE_OPEN, SECMODE_OPEN); 
  keymapChar.setMaxLen(512); 
  keymapChar.setWriteCallback(keymap_write_callback);
  keymapChar.begin(); 
  
  configChar.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP | CHR_PROPS_READ);
  configChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  configChar.setMaxLen(sizeof(Config));
  configChar.setWriteCallback(config_write_callback);
  configChar.begin();
  
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE); 
  Bluefruit.Advertising.addTxPower(); 
  Bluefruit.Advertising.addService(keymapService); 
  Bluefruit.ScanResponse.addName(); 
  Bluefruit.ScanResponse.addAppearance(BLE_APPEARANCE_HID_KEYBOARD); 
  Bluefruit.ScanResponse.addService(blehid); 
  Bluefruit.Advertising.restartOnDisconnect(true); 
  Bluefruit.Advertising.setInterval(32, 244); 
  Bluefruit.Advertising.setFastTimeout(30); 
}

bool processAllInputs() {
  uint8_t kb_modifiers = 0, kb_keycodes[6] = {0}, kb_keycount = 0;
  uint8_t mouse_buttons = 0;
  int8_t mouse_scroll = 0, mouse_pan = 0;
  bool modeSwitchPressedNow = false, activity = false;

  active_layer = 0;
  for (int r = 0; r < ROWS; r++) { for (int c = 0; c < COLS; c++) { if (keyState[r][c]) { int idx = matrix_to_layout_index[r][c]; if (idx != -1 && current_keymap[0][idx] == L_LOWER) { active_layer = 1; goto layer_check_done; } } } }
  layer_check_done:;

  auto process_a_key = [&](uint16_t key) {
    switch (key) {
      case KC_NO: case L_LOWER: break;
      case SW_USB_BT: modeSwitchPressedNow = true; break;
      case KC_MS_UP:    mouseY_accumulator -= MOUSE_MOVE_SPEED; break;
      case KC_MS_DOWN:  mouseY_accumulator += MOUSE_MOVE_SPEED; break;
      case KC_MS_LEFT:  mouseX_accumulator -= MOUSE_MOVE_SPEED; break;
      case KC_MS_RIGHT: mouseX_accumulator += MOUSE_MOVE_SPEED; break;
      case KC_MS_BTN1:  mouse_buttons |= MOUSE_BUTTON_LEFT;   break;
      case KC_MS_BTN2:  mouse_buttons |= MOUSE_BUTTON_RIGHT;  break;
      case KC_MS_BTN3:  mouse_buttons |= MOUSE_BUTTON_MIDDLE; break;
      case KC_MS_BTN4:  mouse_buttons |= MOUSE_BUTTON_BACKWARD; break;
      case KC_MS_BTN5:  mouse_buttons |= MOUSE_BUTTON_FORWARD;  break;
      
      // =========================================================================
      // === ★★★ ここからが今回の修正点です ★★★
      // =========================================================================
      case KC_RESET_KM:
      { // スコープを開始
        for(int i=0; i<5; i++){ setXiaoLed(255,0,255); delay(100); setXiaoLed(0,0,0); delay(100); }
        InternalFS.remove(KEYMAP_FILENAME); 
        InternalFS.remove(CONFIG_FILENAME); 
        delay(100); 
        NVIC_SystemReset(); 
        break;
      } // スコープを終了
      case KC_REBOOT_DEF:
      { // スコープを開始
        for(int i=0; i<3; i++){ setXiaoLed(0,0,255); delay(50); setXiaoLed(0,0,0); delay(50); }
        File flagFile = InternalFS.open(BOOT_FLAG_FILENAME, FILE_O_WRITE); 
        if (flagFile) {
            flagFile.close(); 
        }
        delay(100); 
        NVIC_SystemReset(); 
        break;
      } // スコープを終了
      // =========================================================================

      default:
        if (key >= HID_KEY_CONTROL_LEFT && key <= HID_KEY_GUI_RIGHT) { kb_modifiers |= (1 << (key - HID_KEY_CONTROL_LEFT)); } 
        else if (kb_keycount < 6) { kb_keycodes[kb_keycount++] = key; }
        break;
    }
  };

  for (int r=0; r<ROWS; r++) { for (int c=0; c<COLS; c++) { if (keyState[r][c]) { int idx = matrix_to_layout_index[r][c]; if (idx != -1) { uint16_t key = current_keymap[active_layer][idx]; if (key == KC_TRNS && active_layer > 0) key = current_keymap[0][idx]; process_a_key(key); } } } }
  if (nextBtnState) { uint16_t key = current_keymap[active_layer][63]; if (key == KC_TRNS && active_layer > 0) key = current_keymap[0][63]; process_a_key(key); }
  if (backBtnState) { uint16_t key = current_keymap[active_layer][64]; if (key == KC_TRNS && active_layer > 0) key = current_keymap[0][64]; process_a_key(key); }

  if (modeSwitchPressedNow && !modeSwitchKeyWasPressed) { if (currentMode == MODE_BLUETOOTH) { if (TinyUSBDevice.mounted()) switchMode(MODE_USB); } else switchMode(MODE_BLUETOOTH); }
  modeSwitchKeyWasPressed = modeSwitchPressedNow;
  
  static unsigned long lastMoveTime = 0;
  if (millis() - lastMoveTime >= 1) {
    lastMoveTime = millis();
    xTotal -= xReadings[readIndex]; yTotal -= yReadings[readIndex];
    xReadings[readIndex] = analogRead(STICK_Y_PIN); yReadings[readIndex] = analogRead(STICK_X_PIN);
    xTotal += xReadings[readIndex]; yTotal += yReadings[readIndex];
    readIndex = (readIndex + 1) % currentConfig.smoothing_samples;
    int xAvg = xTotal / currentConfig.smoothing_samples, yAvg = yTotal / currentConfig.smoothing_samples;
    int xDiff = xAvg - currentConfig.stick_center_x, yDiff = currentConfig.stick_center_y - yAvg;
    
    if (active_layer == 1) {
      if (abs(yDiff) > currentConfig.stick_deadzone && abs(yDiff) > currentConfig.scroll_min_threshold) {
        float magnitude = abs(yDiff);
        float scroll_speed = (magnitude > currentConfig.scroll_max_threshold) ? active_scroll_max_speed : active_scroll_min_speed + pow((magnitude - currentConfig.scroll_min_threshold) / (currentConfig.scroll_max_threshold - currentConfig.scroll_min_threshold), 2) * (active_scroll_max_speed - active_scroll_min_speed);
        scroll_accumulator += (yDiff > 0) ? -scroll_speed : scroll_speed;
      } else scroll_accumulator = 0;
      
      if (abs(xDiff) > currentConfig.stick_deadzone && abs(xDiff) > currentConfig.scroll_min_threshold) {
        float magnitude = abs(xDiff);
        float pan_speed = (magnitude > currentConfig.scroll_max_threshold) ? active_scroll_max_speed : active_scroll_min_speed + pow((magnitude - currentConfig.scroll_min_threshold) / (currentConfig.scroll_max_threshold - currentConfig.scroll_min_threshold), 2) * (active_scroll_max_speed - active_scroll_min_speed);
        pan_accumulator += (xDiff > 0) ? pan_speed : -pan_speed;
      } else pan_accumulator = 0;
      
      if (abs(scroll_accumulator) >= 0.5f) { mouse_scroll = (int8_t)round(scroll_accumulator); scroll_accumulator -= mouse_scroll; }
      if (abs(pan_accumulator) >= 0.5f) { mouse_pan = (int8_t)round(pan_accumulator); pan_accumulator -= mouse_pan; }
      mouseX_accumulator = mouseY_accumulator = 0;
    } else {
      scroll_accumulator = pan_accumulator = 0;
      float speed_magnitude = max(abs(xDiff), abs(yDiff));
      if (speed_magnitude > currentConfig.stick_deadzone) {
        float dir_mag = sqrt(pow(xDiff, 2) + pow(yDiff, 2)); 
        if (dir_mag < 1) dir_mag = 1; 
        float speed = (speed_magnitude - currentConfig.stick_deadzone) / (511.0f - currentConfig.stick_deadzone) * active_mouse_high_speed;
        mouseX_accumulator += (xDiff / dir_mag) * speed; 
        mouseY_accumulator += (yDiff / dir_mag) * speed;
      } else mouseX_accumulator = mouseY_accumulator = 0;
    }
  }
  
  int8_t mouseX_to_send = 0, mouseY_to_send = 0;
  if (abs(mouseX_accumulator) >= 0.5f) { mouseX_to_send = (int8_t)round(mouseX_accumulator); mouseX_accumulator -= mouseX_to_send; }
  if (abs(mouseY_accumulator) >= 0.5f) { mouseY_to_send = (int8_t)round(mouseY_accumulator); mouseY_accumulator -= mouseY_to_send; }
  
  if (kb_modifiers != prev_kb_modifiers || memcmp(kb_keycodes, prev_kb_keycodes, 6) != 0) {
    sendKeyboardReport(kb_modifiers, kb_keycodes);
    prev_kb_modifiers = kb_modifiers; memcpy(prev_kb_keycodes, kb_keycodes, 6); activity = true;
  }
  
  sendMouseReport(mouse_buttons, mouseX_to_send, mouseY_to_send, mouse_scroll, mouse_pan);
  if (mouseX_to_send != 0 || mouseY_to_send != 0 || mouse_scroll != 0 || mouse_pan != 0 || mouse_buttons != prev_mouse_buttons) {
    prev_mouse_buttons = mouse_buttons; activity = true;
  }
  
  return activity;
}

void scanMatrix() { 
  unsigned long currentTime = millis(); 
  for (int c = 0; c < COLS; c++) {
    mcp.writeGPIOAB(~(1 << (c + 8)));
    delayMicroseconds(5);
    uint16_t gpioState = mcp.readGPIOAB();
    for (int r = 0; r < ROWS; r++) { 
      bool rawState = !(gpioState & (1 << r));
      if (rawState != lastRawKeyState[r][c]) { keyChangeTime[r][c] = currentTime; lastRawKeyState[r][c] = rawState; }
      if ((currentTime - keyChangeTime[r][c]) > debounceDelay) keyState[r][c] = rawState; 
    }
  }
  mcp.writeGPIOAB(0xFFFF);
}

void scanDirectKeys() { 
  unsigned long currentTime = millis(); 
  bool rawBackState = (digitalRead(BACK_BTN_PIN) == LOW); 
  if (rawBackState != lastRawBackBtnState) { backBtnChangeTime = currentTime; lastRawBackBtnState = rawBackState; }
  if ((currentTime - backBtnChangeTime) > debounceDelay) backBtnState = rawBackState; 
  bool rawNextState = (digitalRead(NEXT_BTN_PIN) == LOW); 
  if (rawNextState != lastRawNextBtnState) { nextBtnChangeTime = currentTime; lastRawNextBtnState = rawNextState; }
  if ((currentTime - nextBtnChangeTime) > debounceDelay) nextBtnState = rawNextState; 
}

void switchMode(OperatingMode_t newMode) { 
  currentMode = newMode;
  applyModeConfig(newMode);
   
  if (newMode == MODE_USB) { 
    if (Bluefruit.connected()) Bluefruit.disconnect(Bluefruit.connHandle());
    Bluefruit.Advertising.stop(); 
    setXiaoLed(0, currentConfig.led_brightness, 0); 
    analogWrite(BLUETOOTH_LED_PIN, 0);
  } else { 
    Bluefruit.Advertising.start(0); 
    setXiaoLed(0, 0, currentConfig.led_brightness); 
  } 
  lastActivityTime = millis(); 
}

void sendKeyboardReport(uint8_t m, uint8_t k[6]) { 
  if (currentMode == MODE_USB && TinyUSBDevice.mounted() && usb_hid.ready()) usb_hid.keyboardReport(1, m, k);
  else if (currentMode == MODE_BLUETOOTH && Bluefruit.connected()) blehid.keyboardReport(m, k); 
}

void sendMouseReport(uint8_t b, int8_t x, int8_t y, int8_t s, int8_t p) { 
  if (currentMode == MODE_USB && TinyUSBDevice.mounted() && usb_hid.ready()) usb_hid.mouseReport(2, b, x, y, s, p);
  else if (currentMode == MODE_BLUETOOTH && Bluefruit.connected()) blehid.mouseReport(b, x, y, s, p); 
}

bool initMCP23017() { 
  if (!mcp.begin_I2C()) return false; 
  for (int i = 0; i < 8; i++) mcp.pinMode(i, INPUT_PULLUP); 
  for (int i = 8; i < 16; i++) mcp.pinMode(i, OUTPUT); 
  mcp.writeGPIOAB(0xFFFF);
  return true; 
}

bool isAnyActivity() { 
  for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) if (keyState[r][c]) return true; 
  if (backBtnState || nextBtnState) return true; 
  if (abs(analogRead(STICK_Y_PIN) - currentConfig.stick_center_x) > currentConfig.stick_deadzone || abs(analogRead(STICK_X_PIN) - currentConfig.stick_center_y) > currentConfig.stick_deadzone) return true; 
  return false; 
}

void goToSystemOnSleep() { isSleeping = true; setXiaoLed(0,0,0); analogWrite(POWER_LED_PIN, 0); analogWrite(BLUETOOTH_LED_PIN, 0); }
void wakeUpFromSystemOnSleep() { isSleeping = false; lastActivityTime = millis(); applyConfig(); }
void setXiaoLed(int r, int g, int b) { analogWrite(LED_RED, 255 - r); analogWrite(LED_GREEN, 255 - g); analogWrite(LED_BLUE, 255 - b); }

void loadDefaultConfig() {
  currentConfig = {STICK_CENTER_X, STICK_CENTER_Y, STICK_DEADZONE, MOUSE_HIGH_SPEED, MOUSE_LOW_SPEED, MOUSE_ACCEL_THRESHOLD, SMOOTHING_SAMPLES, SCROLL_MIN_THRESHOLD, SCROLL_MAX_THRESHOLD, SCROLL_MIN_SPEED, SCROLL_MAX_SPEED, SCROLL_INTERVAL_MS, MOUSE_HIGH_SPEED_BT, SCROLL_MIN_SPEED_BT, SCROLL_MAX_SPEED_BT, SLEEP_TIMEOUT, LED_BRIGHTNESS, BLINK_INTERVAL_MS, 0x504F5441};
}

void loadConfigFromFlash() {
  if (InternalFS.exists(CONFIG_FILENAME)) {
    File configFile = InternalFS.open(CONFIG_FILENAME, FILE_O_READ);
    if (configFile) {
      Config tempConfig;
      if (configFile.read((uint8_t*)&tempConfig, sizeof(Config)) == sizeof(Config) && tempConfig.magic == 0x504F5441) {
        memcpy(&currentConfig, &tempConfig, sizeof(Config));
      }
      configFile.close();
    }
  }
}

void saveConfigToFlash() {
  InternalFS.remove(CONFIG_FILENAME);
  File configFile = InternalFS.open(CONFIG_FILENAME, FILE_O_WRITE);
  if (configFile) {
    configFile.write((uint8_t*)&currentConfig, sizeof(Config));
    configFile.close();
  }
}

void applyConfig() {
  applyModeConfig(currentMode);
  if (!isSleeping) {
      analogWrite(POWER_LED_PIN, currentConfig.led_brightness);
      if (currentMode == MODE_BLUETOOTH && Bluefruit.connected()) {
        analogWrite(BLUETOOTH_LED_PIN, currentConfig.led_brightness);
      }
  }
}

void applyModeConfig(OperatingMode_t mode) {
  if (mode == MODE_USB) {
    active_mouse_high_speed = currentConfig.mouse_high_speed;
    active_scroll_min_speed = currentConfig.scroll_min_speed;
    active_scroll_max_speed = currentConfig.scroll_max_speed;
  } else {
    active_mouse_high_speed = currentConfig.mouse_high_speed_bt;
    active_scroll_min_speed = currentConfig.scroll_min_speed_bt;
    active_scroll_max_speed = currentConfig.scroll_max_speed_bt;
  }
}
// =============================================================================
// High-Resolution Mouse Wheel テスト v5
// XIAO nRF52840 / USB HID / CDC なし
//
// Resolution Multiplier (Usage 0x48) で 1/120 notch 単位のスクロールを実現。
// Windows が SET_FEATURE で hi-res を有効化すると緑 LED が点灯。
//
// LED:
//   赤3回        → 起動
//   青点灯       → USB マウント済み
//   緑点灯       → Windows が hi-res モードを有効化 (Resolution Multiplier = 120)
//   赤点滅       → マウント失敗
// =============================================================================

#include <Adafruit_TinyUSB.h>

// --- ピン定義 ---
#define STICK_Y_PIN    A2
#define STICK_CENTER   512
#define STICK_DEADZONE 75
#define STICK_RANGE    450

// --- スクロール速度 (notches/sec、全開) ---
#define SCROLL_MAX_SPEED  20.0f

// --- LED ---
#define LED_R  LED_RED
#define LED_G  LED_GREEN
#define LED_B  LED_BLUE

// ---------------------------------------------------------------------------
// HID descriptor: Mouse + Resolution Multiplier (Logical Collection 内)
// Input  Report ID 1: buttons(1B) + x(2B) + y(2B) + wheel(2B) = 7B
// Feature Report ID 1: resolution_multiplier(2bit) + padding(6bit) = 1B
// ---------------------------------------------------------------------------
static const uint8_t desc_hid_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,      //   Report ID (1)
    0x09, 0x01,      //   Usage (Pointer)
    0xA1, 0x00,      //   Collection (Physical)
      // --- Buttons: 5 ボタン + 3bit padding ---
      0x05, 0x09,    //     Usage Page (Button)
      0x19, 0x01,    //     Usage Min (1)
      0x29, 0x05,    //     Usage Max (5)
      0x15, 0x00,    //     Logical Min (0)
      0x25, 0x01,    //     Logical Max (1)
      0x95, 0x05,    //     Report Count (5)
      0x75, 0x01,    //     Report Size (1)
      0x81, 0x02,    //     Input (Data, Var, Abs)
      0x95, 0x01,    //     Report Count (1)
      0x75, 0x03,    //     Report Size (3)
      0x81, 0x03,    //     Input (Const)
      // --- X, Y: 16bit relative ---
      0x05, 0x01,    //     Usage Page (Generic Desktop)
      0x09, 0x30,    //     Usage (X)
      0x09, 0x31,    //     Usage (Y)
      0x16, 0x01, 0x80,  // Logical Min (-32767)
      0x26, 0xFF, 0x7F,  // Logical Max (32767)
      0x95, 0x02,    //     Report Count (2)
      0x75, 0x10,    //     Report Size (16)
      0x81, 0x06,    //     Input (Data, Var, Rel)
      // --- Wheel + Resolution Multiplier ---
      0x09, 0x38,    //     Usage (Wheel)  ← Logical Collection の外側に置く
      0xA1, 0x02,    //     Collection (Logical)
        // Feature: Resolution Multiplier
        0x05, 0x01,  //       Usage Page (Generic Desktop)
        0x09, 0x48,  //       Usage (Resolution Multiplier)
        0x15, 0x00,  //       Logical Min (0)
        0x25, 0x01,  //       Logical Max (1)
        0x35, 0x01,  //       Physical Min (1)
        0x45, 0x78,  //       Physical Max (120)
        0x95, 0x01,  //       Report Count (1)
        0x75, 0x02,  //       Report Size (2)
        0xB1, 0x02,  //       Feature (Data, Var, Abs)
        0x95, 0x01,  //       Report Count (1)
        0x75, 0x06,  //       Report Size (6)
        0xB1, 0x03,  //       Feature (Const, padding)
        // Input: Wheel 16bit
        0x05, 0x01,  //       Usage Page (Generic Desktop)
        0x09, 0x38,  //       Usage (Wheel)
        0x16, 0x01, 0x80,  // Logical Min (-32767)
        0x26, 0xFF, 0x7F,  // Logical Max (32767)
        0x35, 0x00,  //       Physical Min (0) [reset]
        0x45, 0x00,  //       Physical Max (0) [reset]
        0x95, 0x01,  //       Report Count (1)
        0x75, 0x10,  //       Report Size (16)
        0x81, 0x06,  //       Input (Data, Var, Rel)
      0xC0,          //     End Collection (Logical)
    0xC0,            //   End Collection (Physical)
  0xC0,              // End Collection (Application)
};

// Input Report 構造体 (report_id なし、sendReport で ID を渡す)
struct __attribute__((packed)) MouseReport {
  uint8_t  buttons;
  int16_t  x;
  int16_t  y;
  int16_t  wheel;   // hi-res: 1 unit = 1/120 notch
};

Adafruit_USBD_HID usb_hid;

// Windows が SET_FEATURE で設定した解像度倍率 (1 or 120)
volatile uint8_t g_resolution_mult = 1;

// ---------------------------------------------------------------------------
// GET_REPORT callback (Windows が Feature の現在値を問い合わせ)
// ---------------------------------------------------------------------------
uint16_t get_report_cb(uint8_t report_id,
                        hid_report_type_t report_type,
                        uint8_t* buffer, uint16_t req_len) {
  if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 1) {
    buffer[0] = (g_resolution_mult == 120) ? 0x01 : 0x00;
    return 1;
  }
  return 0;
}

// ---------------------------------------------------------------------------
// SET_REPORT callback (Windows が hi-res モードを有効化)
// ---------------------------------------------------------------------------
void set_report_cb(uint8_t report_id,
                    hid_report_type_t report_type,
                    uint8_t const* buffer, uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 1 && bufsize >= 1) {
    uint8_t logical = buffer[0] & 0x03;   // 2bit 値
    if (logical == 1) {
      g_resolution_mult = 120;
      digitalWrite(LED_G, LOW);   // 緑: hi-res 有効
    } else {
      g_resolution_mult = 1;
      digitalWrite(LED_G, HIGH);
    }
  }
}

// ---------------------------------------------------------------------------

float smoothed_y = STICK_CENTER;
float acc_scroll = 0.0f;
unsigned long lastTime = 0;

void setup() {
  pinMode(LED_R, OUTPUT); digitalWrite(LED_R, HIGH);
  pinMode(LED_G, OUTPUT); digitalWrite(LED_G, HIGH);
  pinMode(LED_B, OUTPUT); digitalWrite(LED_B, HIGH);

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_R, LOW);  delay(150);
    digitalWrite(LED_R, HIGH); delay(150);
  }

  TinyUSBDevice.setID(0x239A, 0x802F);
  TinyUSBDevice.setProductDescriptor("PotaKB HiRes Mouse v5");

  usb_hid.setBootProtocol(HID_ITF_PROTOCOL_MOUSE);
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setReportCallback(get_report_cb, set_report_cb);
  usb_hid.begin();

  unsigned long t = millis();
  while (!TinyUSBDevice.mounted() && millis() - t < 5000) delay(10);

  if (TinyUSBDevice.mounted()) {
    digitalWrite(LED_B, LOW);   // 青: マウント OK
  } else {
    for (;;) {
      digitalWrite(LED_R, LOW);  delay(50);
      digitalWrite(LED_R, HIGH); delay(50);
    }
  }

  smoothed_y = (float)analogRead(STICK_Y_PIN);
  lastTime = millis();
}

void loop() {
  if (!TinyUSBDevice.mounted()) { delay(10); return; }

  unsigned long now = millis();
  float dt = (float)(now - lastTime);
  if (dt < 1.0f) { delay(1); return; }
  lastTime = now;

  // --- スティック読み取り ---
  float raw_y = (float)analogRead(STICK_Y_PIN);
  smoothed_y  = 0.4f * raw_y + 0.6f * smoothed_y;
  float diff_y = smoothed_y - STICK_CENTER;
  if (fabsf(diff_y) < STICK_DEADZONE) diff_y = 0.0f;

  float sv = 0.0f;
  if (diff_y != 0.0f) {
    float norm = (fabsf(diff_y) - STICK_DEADZONE) / (float)(STICK_RANGE - STICK_DEADZONE);
    norm = constrain(norm, 0.0f, 1.0f);
    sv = copysignf(norm * norm * SCROLL_MAX_SPEED, diff_y);
  }

  // acc は常に「notch 単位」で積算
  acc_scroll += sv * dt * 0.001f;
  if (fabsf(acc_scroll) > 30.0f) acc_scroll = copysignf(30.0f, acc_scroll);

  // --- 送信 ---
  uint8_t mult = g_resolution_mult;
  int16_t send_val = 0;

  if (mult == 120) {
    // hi-res: 1/120 notch 単位で送信
    send_val = (int16_t)constrain((int)(acc_scroll * 120.0f), -32767, 32767);
  } else {
    // lo-res: 整数 notch 単位
    send_val = (int16_t)constrain((int)acc_scroll, -127, 127);
  }

  if (send_val != 0 && usb_hid.ready()) {
    MouseReport report = {0, 0, 0, send_val};
    bool ok = usb_hid.sendReport(1, &report, sizeof(report));
    if (ok) {
      if (mult == 120) {
        acc_scroll -= send_val / 120.0f;
      } else {
        acc_scroll -= (float)send_val;
      }
    }
  }
}

// =========================================================================
// ★★★ あなたが編集するのは、このファイルだけ! ★★★
// =========================================================================

#include <Adafruit_TinyUSB.h>

// --- アナログスティック設定 (USB用) ---
#define STICK_CENTER_X       470
#define STICK_CENTER_Y       470
#define STICK_DEADZONE       75
#define MOUSE_HIGH_SPEED     3.6f // ★ 最高速度
#define MOUSE_LOW_SPEED      0.0f  // ★ 低速ゾーンの最高速度
#define SMOOTHING_SAMPLES    2
#define MOUSE_ACCEL_THRESHOLD 400 // ★ 低速ゾーンが終わる傾き

// --- スクロール設定 (USB用) ---
#define SCROLL_MIN_THRESHOLD 80    // 反応開始点を下げて早く反応
#define SCROLL_MAX_THRESHOLD 350   // 最大速度到達点
#define SCROLL_MIN_SPEED 0.05f      // 最小速度を上げる
#define SCROLL_MAX_SPEED 0.2f      // 最大速度を大幅に上げる
#define SCROLL_INTERVAL_MS 10      // スクロール更新間隔

// --- Bluetooth専用設定 ---
#define MOUSE_HIGH_SPEED_BT  60.0f  // ★ Bluetooth時のマウス最高速度
#define SCROLL_MIN_SPEED_BT  0.15f  // ★ Bluetooth時のスクロール最小速度
#define SCROLL_MAX_SPEED_BT  1.2f   // ★ Bluetooth時のスクロール最大速度

// --- 電源管理設定 ---
#define SLEEP_TIMEOUT 300000UL // スリープまでの時間(ms)
#define LED_BRIGHTNESS 25
#define BLINK_INTERVAL_MS 600

// --- キーマトリクス設定 ---
#define ROWS 8
#define COLS 8
#define LAYOUT_KEY_COUNT 65

// --- カスタムキーコード定義 ---
#define KC_MS_UP         0x201
#define KC_MS_DOWN       0x202
#define KC_MS_LEFT       0x203
#define KC_MS_RIGHT      0x204
#define KC_MS_BTN1       0x205 // 左クリック
#define KC_MS_BTN2       0x206 // 右クリック
#define KC_MS_BTN3       0x207 // 中クリック
#define KC_MS_BTN4       0x208 // 戻る
#define KC_MS_BTN5       0x209 // 進む
#define MOUSE_MOVE_SPEED 8
#define KC_NO            0x000
#define SW_USB_BT        0x401

#define KC_RESET_KM      0x501
#define KC_REBOOT_DEF    0x502

// JP Keyboard specific keycodes
#ifndef HID_KEY_INTERNATIONAL1
  #define HID_KEY_INTERNATIONAL1 0x89
#endif
#ifndef HID_KEY_INTERNATIONAL3
  #define HID_KEY_INTERNATIONAL3 0x87
#endif
#ifndef HID_KEY_LANG1
  #define HID_KEY_LANG1 0x90
#endif
#ifndef HID_KEY_LANG2
  #define HID_KEY_LANG2 0x91
#endif

// レイヤー機能用
#define NUM_LAYERS 2
#define L_LOWER    0x301
#define KC_TRNS    0x000


// --- キーマップレイアウト定義 ---
const uint16_t layout[NUM_LAYERS][LAYOUT_KEY_COUNT] = {
  // [0] = Default Layer
  {
    // 1段目
    KC_MS_BTN1, KC_MS_BTN2,
    // 2段目
    HID_KEY_ESCAPE, HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7, HID_KEY_8, HID_KEY_9, HID_KEY_0, HID_KEY_MINUS, HID_KEY_EQUAL, HID_KEY_INTERNATIONAL1, HID_KEY_DELETE,
    // 3段目
    HID_KEY_GRAVE, HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R, HID_KEY_T, HID_KEY_Y, HID_KEY_U, HID_KEY_I, HID_KEY_O, HID_KEY_P, HID_KEY_BRACKET_LEFT, HID_KEY_BRACKET_RIGHT, HID_KEY_BACKSPACE,
    // 4段目
    HID_KEY_TAB, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G, HID_KEY_H, HID_KEY_J, HID_KEY_K, HID_KEY_L, HID_KEY_SEMICOLON, HID_KEY_APOSTROPHE, HID_KEY_BACKSLASH, HID_KEY_ENTER,
    // 5段目
    HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B, HID_KEY_N, HID_KEY_M, HID_KEY_COMMA, HID_KEY_PERIOD, HID_KEY_SLASH, HID_KEY_INTERNATIONAL3, HID_KEY_SHIFT_LEFT,
    // 6段目
    HID_KEY_SPACE, HID_KEY_GUI_LEFT, HID_KEY_ALT_LEFT, HID_KEY_CONTROL_LEFT, KC_MS_BTN3, L_LOWER,
    // 直接接続キー
    KC_MS_BTN4, KC_MS_BTN5,
  },

  // [1] = Lower Layer (Fnキーを押している間)
  {
    // 1段目
    KC_NO, KC_NO,
    // 2段目
    SW_USB_BT, HID_KEY_F1, HID_KEY_F2, HID_KEY_F3, HID_KEY_F4, HID_KEY_F5, HID_KEY_F6, HID_KEY_F7, HID_KEY_F8, HID_KEY_F9, HID_KEY_F10, HID_KEY_F11, HID_KEY_F12, KC_NO, KC_NO,
    // 3段目
    KC_NO, KC_NO, HID_KEY_ARROW_UP, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    // 4段目
    KC_NO, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_RIGHT, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    // 5段目
    HID_KEY_END, HID_KEY_HOME, HID_KEY_PAGE_UP, HID_KEY_PAGE_DOWN, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    // 6段目
    KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, L_LOWER,
    // 直接接続キー
    KC_RESET_KM, KC_REBOOT_DEF,
  }
};

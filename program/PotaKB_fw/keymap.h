// =========================================================================
// ★★★ あなたが編集するのは、このファイルだけ! ★★★
// =========================================================================

#include <Adafruit_TinyUSB.h>

// --- アナログスティック設定 (黄金設定) ---
#define STICK_CENTER_X       470
#define STICK_CENTER_Y       470
#define STICK_DEADZONE       75
#define MOUSE_HIGH_SPEED     75.0f // ★ 最高速度
#define MOUSE_LOW_SPEED      10.0f  // ★ 低速ゾーンの最高速度
#define SMOOTHING_SAMPLES    2
#define MOUSE_ACCEL_THRESHOLD 400 // ★ 低速ゾーンが終わる傾き

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

// ★★★ 新しいカスタムキーコード ★★★
#define KC_RESET_KM      0x501
#define KC_REBOOT_DEF    0x502

#ifndef HID_KEY_INTERNATIONAL1
  #define HID_KEY_INTERNATIONAL1 0x87 // ¥キー
#endif
#ifndef HID_KEY_INTERNATIONAL3
  #define HID_KEY_INTERNATIONAL3 0x89 // _ (アンダースコア) キー
#endif
#ifndef HID_KEY_LANG1
  #define HID_KEY_LANG1 0x90 // 半角/全角・漢字キー
#endif
#ifndef HID_KEY_LANG2
  #define HID_KEY_LANG2 0x91 // カタカナ・ひらがな・ローマ字キー
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
    KC_TRNS, KC_TRNS,
    // 2段目
    SW_USB_BT, HID_KEY_F1, HID_KEY_F2, HID_KEY_F3, HID_KEY_F4, HID_KEY_F5, HID_KEY_F6, HID_KEY_F7, HID_KEY_F8, HID_KEY_F9, HID_KEY_F10, HID_KEY_F11, HID_KEY_F12, KC_TRNS, KC_TRNS,
    // 3段目
    KC_TRNS, KC_TRNS, HID_KEY_ARROW_UP, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
    // 4段目
    KC_TRNS, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_RIGHT, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
    // 5段目
    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
    // 6段目
    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, L_LOWER,
    // 直接接続キー
    KC_RESET_KM, KC_REBOOT_DEF,
  }
};

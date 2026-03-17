// =========================================================================
// PotaKB keymap.h
// ★ ここを編集してキーマップをカスタマイズ ★
// =========================================================================

#pragma once
#include <Adafruit_TinyUSB.h>

// -------------------------------------------------------------------------
// ハードウェア定数（変更不要）
// -------------------------------------------------------------------------
#define ROWS              8
#define COLS              8
#define LAYOUT_KEY_COUNT  65   // 63(matrix) + 2(direct: Back/Next)
#define NUM_LAYERS        2

// -------------------------------------------------------------------------
// キーコード定義
// -------------------------------------------------------------------------

// 何もしない / 透過
#define KC_NO    0x0000   // 何もしない
#define KC_TRNS  0xFFFF   // 下のレイヤーに透過（KC_NOとは別値）

// レイヤー操作
// MO(n): 押している間レイヤーnをアクティブにする
#define MO(n)   (0x5000 | (n))

// USB / Bluetoothモード切替
// SW_MODE: 押すたびに USB→BT1→BT2→BT3→USB と切替
#define SW_MODE  0x4001

// スティックキャリブレーション
#define KC_CALIBRATE  0x4002

// リセット系
#define KC_RESET   0x4003   // キーマップ・設定リセット＋再起動
#define KC_REBOOT  0x4004   // 設定保持のまま再起動

// マウスキー
#define KC_MS_UP    0x2001
#define KC_MS_DOWN  0x2002
#define KC_MS_LEFT  0x2003
#define KC_MS_RIGHT 0x2004
#define KC_MS_BTN1  0x2005   // 左クリック
#define KC_MS_BTN2  0x2006   // 右クリック
#define KC_MS_BTN3  0x2007   // 中クリック
#define KC_MS_BTN4  0x2008   // 戻る
#define KC_MS_BTN5  0x2009   // 進む
#define KC_MS_SCR_UP    0x200A
#define KC_MS_SCR_DOWN  0x200B
#define KC_MS_SCR_LEFT  0x200C
#define KC_MS_SCR_RIGHT 0x200D

// 日本語キーボード固有キー（未定義の場合のみ定義）
#ifndef HID_KEY_INTERNATIONAL1
  #define HID_KEY_INTERNATIONAL1 0x89  // ろ（\ |）
#endif
#ifndef HID_KEY_INTERNATIONAL3
  #define HID_KEY_INTERNATIONAL3 0x87  // ¥ |
#endif
#ifndef HID_KEY_LANG1
  #define HID_KEY_LANG1 0x90  // かな
#endif
#ifndef HID_KEY_LANG2
  #define HID_KEY_LANG2 0x91  // 英数
#endif

// -------------------------------------------------------------------------
// デフォルト設定値
// -------------------------------------------------------------------------
#define DEFAULT_STICK_CENTER_X    512
#define DEFAULT_STICK_CENTER_Y    512
#define DEFAULT_STICK_DEADZONE    75
#define DEFAULT_STICK_EMA_ALPHA   0.4f   // スムージング係数 (0=最大平滑, 1=なし)
#define DEFAULT_MOUSE_MAX_SPEED   0.8f   // px/ms（deltaTime方式）
#define DEFAULT_SCROLL_MAX_SPEED  0.06f  // tick/ms
#define DEFAULT_SLEEP_TIMEOUT_MS  300000UL
#define DEFAULT_LED_BRIGHTNESS    25
#define DEFAULT_BLINK_INTERVAL_MS 600

// -------------------------------------------------------------------------
// デフォルトキーマップ
// キー順序: 物理レイアウト順（左上から右下）
//
// [0..1]   : 1段目（マウスボタン2個）
// [2..16]  : 2段目（数字行 15キー）
// [17..30] : 3段目（Q行 14キー）
// [31..44] : 4段目（A行 14キー）
// [45..56] : 5段目（Z行 12キー）
// [57..62] : 6段目（6キー）
// [63]     : Nextボタン（D10）
// [64]     : Backボタン（D6）
// -------------------------------------------------------------------------
const uint16_t default_keymap[NUM_LAYERS][LAYOUT_KEY_COUNT] = {

  // =======================================================================
  // [0] Base Layer
  // =======================================================================
  {
    // 1段目 (2キー)
    KC_MS_BTN1, KC_MS_BTN2,

    // 2段目 (15キー)
    HID_KEY_ESCAPE,
    HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5,
    HID_KEY_6, HID_KEY_7, HID_KEY_8, HID_KEY_9, HID_KEY_0,
    HID_KEY_MINUS, HID_KEY_EQUAL,
    HID_KEY_INTERNATIONAL1,  // ろ
    HID_KEY_DELETE,

    // 3段目 (14キー)
    HID_KEY_GRAVE,
    HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R, HID_KEY_T,
    HID_KEY_Y, HID_KEY_U, HID_KEY_I, HID_KEY_O, HID_KEY_P,
    HID_KEY_BRACKET_LEFT, HID_KEY_BRACKET_RIGHT,
    HID_KEY_BACKSPACE,

    // 4段目 (14キー)
    HID_KEY_TAB,
    HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G,
    HID_KEY_H, HID_KEY_J, HID_KEY_K, HID_KEY_L,
    HID_KEY_SEMICOLON, HID_KEY_APOSTROPHE,
    HID_KEY_BACKSLASH,
    HID_KEY_ENTER,

    // 5段目 (12キー)
    HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B,
    HID_KEY_N, HID_KEY_M,
    HID_KEY_COMMA, HID_KEY_PERIOD, HID_KEY_SLASH,
    HID_KEY_INTERNATIONAL3,  // ¥
    HID_KEY_SHIFT_LEFT,

    // 6段目 (6キー)
    HID_KEY_SPACE,
    HID_KEY_GUI_LEFT,
    HID_KEY_ALT_LEFT,
    HID_KEY_CONTROL_LEFT,
    KC_MS_BTN3,
    MO(1),

    // 直接接続キー
    KC_MS_BTN4,  // Next (D10) = 戻る
    KC_MS_BTN5,  // Back  (D6) = 進む
  },

  // =======================================================================
  // [1] Lower Layer
  // =======================================================================
  {
    // 1段目
    KC_NO, KC_NO,

    // 2段目
    SW_MODE,
    HID_KEY_F1,  HID_KEY_F2,  HID_KEY_F3,  HID_KEY_F4,  HID_KEY_F5,
    HID_KEY_F6,  HID_KEY_F7,  HID_KEY_F8,  HID_KEY_F9,  HID_KEY_F10,
    HID_KEY_F11, HID_KEY_F12,
    KC_NO,
    KC_NO,

    // 3段目
    KC_NO,
    KC_NO, HID_KEY_ARROW_UP,   KC_NO, KC_NO, KC_NO,
    KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    KC_NO, KC_NO,
    KC_NO,

    // 4段目
    KC_NO,
    HID_KEY_ARROW_LEFT, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_RIGHT, KC_NO, KC_NO,
    KC_NO, KC_NO, KC_NO, KC_NO,
    KC_NO, KC_NO,
    KC_NO,
    KC_NO,

    // 5段目
    HID_KEY_END, HID_KEY_HOME, HID_KEY_PAGE_UP, HID_KEY_PAGE_DOWN, KC_NO,
    KC_NO, KC_NO,
    KC_NO, KC_NO, KC_NO,
    KC_NO,
    KC_NO,

    // 6段目
    KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
    MO(1),

    // 直接接続キー
    KC_CALIBRATE,  // Next = キャリブレーション
    KC_RESET,      // Back = リセット
  },
};

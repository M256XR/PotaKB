# PotaKB v2.0 ファームウェア仕様書

## 1. ハードウェア構成（固定）

### MCU
- **XIAO nRF52840** (Arm Cortex-M4, nRF52840)

### ピン配置

| 機能 | ピン | 備考 |
|------|------|------|
| スティック（物理X軸） | A2 | PCBミスによりY動作として扱う |
| スティック（物理Y軸） | A3 | PCBミスによりX動作として扱う |
| Backボタン | D6 | INPUT_PULLUP, LOW=押下 |
| Nextボタン | D10 | INPUT_PULLUP, LOW=押下 |
| 電源スイッチ | D7 | INPUT_PULLUP, LOW=ON（HIGHでシステムオフ） |
| パワーLED | D0 | PWM出力 |
| BT状態LED | D1 | PWM出力 |
| 内蔵RGB LED | LED_RED / GREEN / BLUE | 負論理（255=消灯、0=最大輝度） |
| バッテリー電圧 | PIN_VBAT + VBAT_ENABLE | 分圧回路 (1MΩ / 510kΩ) |

### キーマトリクス
- **I/Oエクスパンダ**: MCP23017 (I2C, 400kHz)
- **構成**: 8行 × 8列 = 最大64キー（実装63キー）
  - GPA[0:7]: 行（INPUT_PULLUP）
  - GPB[8:15]: 列（OUTPUT）
- **直接接続キー**: Back (D6), Next (D10) — 計65キー

---

## 2. キーシステム

### 2.1 レイヤー構成

**レイヤー数: 4**（コンパイル時定数 `NUM_LAYERS` で変更可能）

| レイヤー | 名前 | 用途 |
|---------|------|------|
| 0 | Base | 通常入力（英字・数字・記号） |
| 1 | Lower | 矢印キー・ページ操作・F1-F12 |
| 2 | Raise | 将来拡張用（デフォルト全KC_TRNS） |
| 3 | Adjust | 設定系（キャリブレーション・リセット・モード切替） |

**レイヤー優先度**: 番号が大きいほど優先（QMK準拠）
複数レイヤーが同時アクティブな場合、最も番号が大きいアクティブレイヤーから検索。

### 2.2 レイヤー操作キーコード

| キーコード | 値 | 動作 |
|-----------|-----|------|
| `MO(n)` | 0x5000 + n | 押している間だけレイヤーnをアクティブ（QMKのMO相当） |
| `TG(n)` | 0x5010 + n | 押すたびにレイヤーnのON/OFFを切り替え |
| `DF(n)` | 0x5020 + n | デフォルトレイヤーをnに変更（Flash保存） |

### 2.3 特殊キーコード

| キーコード | 値 | 動作 |
|-----------|-----|------|
| `KC_NO` | 0x0000 | 何もしない |
| `KC_TRNS` | 0xFFFF | 透過（下のレイヤーのキーを使用） ※KC_NOと別値 |
| `SW_MODE` | 0x4001 | USB / Bluetoothモード切替 |
| `KC_CALIBRATE` | 0x4002 | スティックキャリブレーション実行 |
| `KC_RESET` | 0x4003 | キーマップ・設定をデフォルトに戻してリセット |
| `KC_REBOOT` | 0x4004 | 単純リセット（設定保持） |

### 2.4 マウスキーコード

| キーコード | 値 | 動作 |
|-----------|-----|------|
| `KC_MS_UP` | 0x2001 | カーソル上移動 |
| `KC_MS_DOWN` | 0x2002 | カーソル下移動 |
| `KC_MS_LEFT` | 0x2003 | カーソル左移動 |
| `KC_MS_RIGHT` | 0x2004 | カーソル右移動 |
| `KC_MS_BTN1` | 0x2005 | 左クリック |
| `KC_MS_BTN2` | 0x2006 | 右クリック |
| `KC_MS_BTN3` | 0x2007 | 中クリック |
| `KC_MS_BTN4` | 0x2008 | 戻る |
| `KC_MS_BTN5` | 0x2009 | 進む |
| `KC_MS_SCR_UP` | 0x200A | スクロールアップ |
| `KC_MS_SCR_DOWN` | 0x200B | スクロールダウン |

### 2.5 デバウンス

- **方式**: タイムスタンプ方式（QMK Eager Debounce準拠）
- **時間**: 5ms（`DEBOUNCE_MS` で変更可能）
- 押下・リリース両方にデバウンスを適用

---

## 3. アナログスティック

### 3.1 軸マッピング（PCBミス補正）

```
analogRead(A2) → 論理Y軸（上下）
analogRead(A3) → 論理X軸（左右）
```

### 3.2 動作モード

**モードA（Base レイヤー）: マウスカーソル**
**モードB（Lower レイヤー）: スクロール**

レイヤーの状態によってモードが切り替わる。

### 3.3 速度計算（deltaTime方式）

ガクガクの根本原因はBLE/USBで報告間隔が違うのに固定倍率を使っていたこと。
実際の経過時間を使うことでBLEでもUSBでも同じ動きになる。

```
// 毎ループ
deltaTime = 現在時刻 - 前回更新時刻  [ms]

stickX = analogRead(A3) - center_x   // 論理X
stickY = analogRead(A2) - center_y   // 論理Y

if abs(stickX) < deadzone → stickX = 0
if abs(stickY) < deadzone → stickY = 0

// 速度 = (傾き / 最大傾き) の二乗 × 最大速度  [px/ms]
speed_x = sign(stickX) * (abs(stickX) / 511)^2 * MOUSE_MAX_SPEED
speed_y = sign(stickY) * (abs(stickY) / 511)^2 * MOUSE_MAX_SPEED

// 変位 = 速度 × 経過時間
accumulator_x += speed_x * deltaTime
accumulator_y += speed_y * deltaTime

// 整数部分だけ送信、小数部分は次回に持ち越し
send_x = trunc(accumulator_x)
send_y = trunc(accumulator_y)
accumulator_x -= send_x
accumulator_y -= send_y
```

### 3.4 スムージング

- **方式**: 指数移動平均（EMA）
  ```
  smoothed = alpha * raw + (1 - alpha) * prev_smoothed
  ```
- **alpha**: 0.0〜1.0（1.0 = スムージングなし）
- デフォルト: 0.5（設定で変更可能）
- リングバッファ方式より実装がシンプルで遅延も少ない

### 3.5 キャリブレーション

**自動キャリブレーション（`KC_CALIBRATE` キー）**:
1. キーを押す
2. 青LEDが点滅 → スティックから手を離す（2秒待機）
3. その間の平均値を中心値として保存
4. 緑LED点灯で完了、Flashに保存

**起動時キャリブレーション不要**: 保存済みの中心値を使用。
工場出荷値: center_x = 512, center_y = 512（実測後に `KC_CALIBRATE` で上書き）

### 3.6 スクロールモード（Lowerレイヤー時）

- Y軸 → 縦スクロール
- X軸 → 横スクロール（パン）
- 速度計算はマウスと同じdeltaTime方式
- スクロール量はfloatアキュムレータで管理（小数点以下持ち越し）

---

## 4. HIDレポート送信

### 4.1 送信タイミング

| モード | キーボード | マウス |
|--------|-----------|--------|
| USB | 変化時のみ | 1ms間隔（1000Hz） |
| Bluetooth | 変化時のみ | 7.5ms間隔（接続間隔に合わせる） |

マウスは変化がなくても定期送信（ホストのカーソル状態を維持するため）。

### 4.2 BLE接続間隔

- **接続間隔**: 7.5ms（min=max=6 units）
  - ※ 旧FW(5ms)よりわずかに緩めて安定性を優先。体感差はほぼなし。
- **TX Power**: +4dBm

---

## 5. 動作モード

### 5.1 モード一覧

| モード | 説明 |
|--------|------|
| USB | TinyUSB HID |
| BT1 | Bluetoothペアリング先1 |
| BT2 | Bluetoothペアリング先2 |
| BT3 | Bluetoothペアリング先3 |

### 5.2 モード切替

- `SW_MODE` キー（1回押すたびに USB→BT1→BT2→BT3→USB と循環）
- 現在のモードをFlashに保存（再起動後も維持）

### 5.3 BT複数台の仕組み

- nRF52840のボンド情報をスロット0〜2に保存
- 各スロットに接続先のアドレス・鍵を保持
- 未ペアリングのスロットに切替えたらアドバタイズ開始（新規ペアリング待ち）
- ペアリング済スロットなら直接再接続

### 5.4 各モードのLED表示

| 状態 | パワーLED | BT LED | RGB LED |
|------|----------|--------|---------|
| USB接続中 | 点灯 | 消灯 | 緑 |
| BT接続中 | 点灯 | 点灯 | 青 |
| BTアドバタイズ中 | 点灯 | 点滅 | 青（点滅） |
| スリープ | 消灯 | 消灯 | 消灯 |
| キャリブレーション中 | — | 点滅 | 青点滅 |
| リセット実行 | — | — | 紫（点滅） |

---

## 6. 電源管理

### 6.1 スリープ

- **System-ON sleep**: 無操作 `SLEEP_TIMEOUT`(ms) 後に移行
  - BLEスタックは維持（Bluetooth接続継続）
  - LED消灯
  - キー入力・スティック操作で即復帰
- **System-OFF sleep**: 電源スイッチ(D7)がHIGHになったとき
  - 完全電源断（`NRF_POWER->SYSTEMOFF = 1`）
  - 電源スイッチを押し直すと再起動

### 6.2 バッテリー

- 計測: 起動時 + 接続時 + 60秒ごと
- BLE Battery Service (BAS) で報告: 5%刻み
- 計算式: ADC値から実電圧を算出し、放電カーブで%変換

---

## 7. Flashストレージ（LittleFS）

| ファイル | 内容 |
|---------|------|
| `/keymap.bin` | カスタムキーマップ（`NUM_LAYERS × LAYOUT_KEY_COUNT × 2` bytes） |
| `/config.bin` | 設定値（Config構造体、マジックナンバーで有効性検証） |

起動時にファイルが存在しない・マジックが不正 → デフォルト値を使用。

### Config構造体（新設計）

```c
struct Config {
  uint8_t  version;              // 構造体バージョン（互換性チェック用）
  uint16_t stick_center_x;       // スティックX中心値
  uint16_t stick_center_y;       // スティックY中心値
  uint16_t stick_deadzone;       // デッドゾーン（ADC値）
  float    stick_ema_alpha;      // スムージング係数 (0.0〜1.0)
  float    mouse_max_speed;      // マウス最大速度 [px/ms]
  float    scroll_max_speed;     // スクロール最大速度 [tick/ms]
  uint32_t sleep_timeout_ms;     // スリープまでの時間
  uint8_t  led_brightness;       // LED輝度 (0〜255)
  uint16_t blink_interval_ms;    // BT点滅間隔
  uint32_t magic;                // 0x504F5441 = "POTA"
};
```

---

## 8. Configurator（新設計）

### 8.1 通信プロトコル

BLE GATT + USB Serialの両対応。

**サービス UUID**: `adaf0001-c332-42a8-93bd-25e905756cb8`

| Characteristic | UUID | 権限 | 内容 |
|---------------|------|------|------|
| Keymap | `adaf0002-...` | READ / WRITE | キーマップバイナリ |
| Config | `adaf0003-...` | READ / WRITE | Config構造体バイナリ |

USBシリアルコマンド（改行区切り）:

```
READ_KEYMAP    → キーマップをバイナリ返送
WRITE_KEYMAP   → 次にバイナリを受信
READ_CONFIG    → Config構造体をバイナリ返送
WRITE_CONFIG   → 次にバイナリを受信
GET_BATTERY    → "BATTERY:xx\n" 返送
CALIBRATE      → キャリブレーション実行
```

### 8.2 Configurator（Web アプリ）

- キーマップエディタ（タブでレイヤー切替）
- Config設定スライダー
- スティックキャリブレーションボタン
- BLE / USB Serial 両対応
- ローカル保存・読み込み

---

## 9. ファイル構成

```
PotaKB_fw/
├── PotaKB_fw.ino     メインファームウェア
├── keymap.h          ハードウェア定数 + デフォルトキーマップ
PotaKB_Configurator/
├── index.html
├── script.js
└── style.css
```

---

## 10. 確定事項

| 項目 | 決定値 | 備考 |
|------|--------|------|
| レイヤー数 | **2** | ハードウェア制約（FNキー1個） |
| マウス速度カーブ | **二乗** | デッドゾーン付近は遅く、端に行くほど速い |
| スクロール切替 | **レイヤー連動** | Lower時スティック=スクロール。KC_MS_SCR_UP/DOWNは全レイヤーで使用可 |
| BTペアリング | **3台切替** | BT1/BT2/BT3 + USB の計4モード |
| 開発環境 | **Arduino + Adafruit nRF52 core** | nRF5 SDK+SoftDeviceのラッパー。USB HIDはTinyUSB |

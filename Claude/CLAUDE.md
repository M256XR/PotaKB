# PotaKB プロジェクト仕様書

> **このファイルは仕様のサマリ。** 変更時は PROGRESS.md の決定事項ログにも必ず記録すること。

## セッション開始手順

1. `git fetch && git status` で差分確認（別PCでの作業が入っている可能性あり）
2. **PROGRESS.md** を読む（現在地・次のタスク・直近の決定事項）
3. このファイル（CLAUDE.md）で仕様を確認
4. 疑問があれば `chat_logs/原文/index.md` で原文の場所を特定
5. 終了処理は **CLOSING.md** を参照

> 仕様の信頼優先順位: 原文ログ > SPEC.md > CLAUDE.md > PROGRESS.md

---

## コンセプト

XIAO nRF52840ベースのワイヤレスキーボード・マウス複合デバイス。
アナログスティックによるマウス操作を特徴とする自作キーボード。

**詳細仕様 → [../SPEC.md](../SPEC.md)**

---

## ハードウェア構成（固定）

| 部品 | 詳細 |
|------|------|
| MCU | Seeed XIAO nRF52840 |
| I/Oエクスパンダ | MCP23017 (I2C 400kHz) |
| キーマトリクス | 8行×8列、63キー実装 |
| 直接接続キー | Back(D6), Next(D10) = 合計65キー |
| アナログスティック | A2(物理X→論理Y), A3(物理Y→論理X) ※PCBミス補正済 |
| 電源スイッチ | D7 (HIGH=オフ) |
| LED | POWER(D0), BT(D1), RGB内蔵 |
| バッテリー | PIN_VBAT + VBAT_ENABLE、分圧 1MΩ/510kΩ |

**スティックX/Y逆配線はPCBミスによるもの。FWで補正。変更不可。**

---

## ファームウェア概要（v2.0）

### ファイル構成

```
program/PotaKB_fw/
├── PotaKB_fw.ino      メインファームウェア
├── keymap.h           キーマップ・デフォルト設定値
└── PotaKB_Configurator/
    ├── index.html
    ├── style.css
    └── script.js
```

### 主要設計決定

| 項目 | 決定内容 | 理由 |
|------|---------|------|
| レイヤー数 | **2固定** | ハードウェア制約（FNキー1個） |
| KC_TRNS | **0xFFFF** | KC_NO(0x0000)と別値にしないとレイヤー透過バグ発生 |
| マウス速度 | **velocity × deltaTime** | BT/USBで送信間隔が違っても動きが一定になる |
| スムージング | **EMA（α=0.4）** | リングバッファより実装シンプル、配列OOBなし |
| BTモード | **BT1/BT2/BT3 + USB 4モード** | SW_MODEキーで循環切替、Flashに保存 |
| スクロール | **Lowerレイヤー時スティック=スクロール** | KC_MS_SCR_UP/DOWNキーは全レイヤーで使用可 |

### キーコード値（重要）

```
KC_NO   = 0x0000  (何もしない)
KC_TRNS = 0xFFFF  (透過 ← KC_NOと別値！)
MO(n)   = 0x5000 | n
SW_MODE = 0x4001
KC_CALIBRATE = 0x4002
KC_RESET     = 0x4003
KC_REBOOT    = 0x4004
KC_MS_*      = 0x2001〜0x200D
HID標準      = USB HID spec値そのまま
```

### Config構造体（packed、34バイト、CONFIG_VERSION=2）

```c
uint8_t  version;           // offset 0
uint16_t stick_center_x;    // offset 1
uint16_t stick_center_y;    // offset 3
uint16_t stick_deadzone;    // offset 5
uint16_t stick_range_x;     // offset 7  スティックX最大変位
uint16_t stick_range_y;     // offset 9  スティックY最大変位
float    stick_ema_alpha;   // offset 11
float    mouse_max_speed;   // offset 15
float    scroll_max_speed;  // offset 19
uint32_t sleep_timeout_ms;  // offset 23
uint8_t  led_brightness;    // offset 27
uint16_t blink_interval_ms; // offset 28
uint32_t magic;             // offset 30 = 0x504F5441 "POTA"
```

### BLE GATT

- Service: `adaf0001-c332-42a8-93bd-25e905756cb8`
- Keymap Char: `adaf0002-...` (260バイト: 2レイヤー×65キー×uint16 LE)
- Config Char: `adaf0003-...` (30バイト)
- BLE接続間隔: 7.5ms (interval=6)

### USB Serial コマンド

```
READ_KEYMAP / WRITE_KEYMAP
READ_CONFIG / WRITE_CONFIG
GET_BATTERY → "BATTERY:xx\n"
CALIBRATE
GET_VERSION → "PotaKB v2.0\n"
READ_STICK  → "STICK:x,y\n"  (論理X=A3, 論理Y=A2)
CALIB_START → "OK\n"  スティック処理を無効化（キャリブ中カーソル防止）
CALIB_END   → "OK\n"  スティック処理を再開
```

---

## Configurator

`program/PotaKB_fw/PotaKB_Configurator/index.html` をChrome/Edgeで開く。

- BLE接続: デバイス名 `PotaKB`
- USB Serial接続: VID=`0x239A` / PID=`0x8029`
- キー選択 → パレットクリック or **「⌨ キー入力」ボタン**で物理キー割り当て

---

## Claudeの役割と作業方針

### やること
- ファームウェアのバグ修正・機能追加
- Configuratorの改善
- 設計判断のサポート・レビュー

### 注意事項
- ハードウェアのX/Y逆配線は修正しない（PCB修正コストが高い）
- Config構造体のオフセットを変更した場合はConfigurator側も必ず更新
- BLE複数台ボンド管理は現在未完全実装（将来拡張ポイント）

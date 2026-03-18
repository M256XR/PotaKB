# PotaKB

XIAO nRF52840ベースのワイヤレスキーボード・マウス複合デバイス。

## 概要

- **MCU**: Seeed XIAO nRF52840
- **キー数**: 65キー（8×8マトリクス × MCP23017 + 直接接続2キー）
- **接続**: Bluetooth LE（3スロット独立切替）/ USB HID
- **スティック**: アナログスティックによるマウス操作・スクロール（EMAスムージング）
- **設定**: Web Configuratorからリアルタイム書き換え（BLE / USB Serial）

## 主な機能

### Bluetooth
- **3スロット独立接続**（PotaKB-1 / PotaKB-2 / PotaKB-3）
  - スロットごとに固有のBLEアドレスとボンドDBを持つ → 同一PCで複数スロット使用可
  - スロット別LEDカラー：BT1=青 / BT2=水色 / BT3=マゼンタ
- **高速通信**: `BANDWIDTH_MAX`（hvn_qsize=3）+ 接続インターバル7.5ms → 約500Hz
- BT中にUSBを刺すと自動でUSBモードに切替

### USB
- HID Resolution Multiplier対応（対応OSで高精度スクロール）
- VID=`0x239A` / PID=`0x8029`

### その他
- 一定時間無操作でLEDオフ（スリープ）、操作で即復帰
- バッテリー残量通知（BLE Battery Service）
- フラッシュへのキーマップ・設定の永続保存

## ディレクトリ構成

```
program/PotaKB_fw/
├── PotaKB_fw.ino          ファームウェア (Arduino / nRF52 core)
├── keymap.h               キーマップ定義・デフォルト設定値
└── PotaKB_Configurator/   Web設定ツール (BLE + USB Serial対応)
    ├── index.html
    ├── style.css
    └── script.js

tools/
├── .arduino-user/         カスタムライブラリ（改造済み Bluefruit52Lib）
├── build.ps1              ビルドスクリプト
├── flash.ps1              フラッシュスクリプト
├── setup.ps1              環境セットアップ
└── arduino-cli.yaml       arduino-cli設定

kicad/                     KiCAD回路図・PCBデータ
stl/                       3Dプリント用STLモデル
SPEC.md                    ファームウェア仕様書
```

## ビルド環境

arduino-cli を使用（Arduino IDE不要）。

### 初回セットアップ

```
tools/1_setup.bat
```

ボードパッケージ（Seeeduino nRF52）と依存ライブラリを自動インストール。

### ビルド・フラッシュ

```
tools/2_build.bat   # コンパイル
tools/3_flash.bat   # USB Serial DFUでフラッシュ
```

### 依存ライブラリ（tools/.arduino-user/libraries に同梱）

- `Bluefruit52Lib`（スロット別ボンドDB対応に改造）
- `Adafruit_MCP23017_Arduino_Library`
- `Adafruit_BusIO`

## Configuratorの使い方

`program/PotaKB_fw/PotaKB_Configurator/index.html` をブラウザ（Chrome / Edge）で開く。

- **Bluetooth接続**: デバイス名 `PotaKB-1` / `PotaKB-2` / `PotaKB-3` で接続
- **USB Serial接続**: VID=`0x239A` / PID=`0x8029` で接続
- キーをクリックして選択し、パレットまたはキー入力で割り当て
- 接続時にバッテリー残量を自動取得

# PotaKB

XIAO nRF52840ベースのワイヤレスキーボード・マウス複合デバイス。

## 概要

- **MCU**: Seeed XIAO nRF52840
- **キー数**: 65キー（8×8マトリクス × MCP23017 + 直接接続2キー）
- **接続**: Bluetooth LE（3台切替）/ USB HID
- **スティック**: アナログスティックによるマウス操作・スクロール
- **設定**: Web Configuratorからリアルタイム書き換え（BLE / USB Serial）

## ディレクトリ構成

```
program/PotaKB_fw/
├── PotaKB_fw.ino          ファームウェア (Arduino / Adafruit nRF52 core)
├── keymap.h               キーマップ定義・デフォルト設定値
└── PotaKB_Configurator/   Web設定ツール (BLE + USB Serial対応)
    ├── index.html
    ├── style.css
    └── script.js

kicad/                     KiCAD回路図・PCBデータ
stl/                       3Dプリント用STLモデル
SPEC.md                    ファームウェア仕様書
```

## ビルド環境

- Arduino IDE + [Adafruit nRF52 Arduino core](https://github.com/adafruit/Adafruit_nRF52_Arduino)
- 依存ライブラリ: `Adafruit_MCP23X17`, `Adafruit_TinyUSB`

## Configuratorの使い方

`program/PotaKB_fw/PotaKB_Configurator/index.html` をブラウザ（Chrome / Edge）で開く。

- **Bluetooth接続**: デバイス名 `PotaKB` で接続
- **USB Serial接続**: VID=`0x239A` / PID=`0x8029` で接続
- キーをクリックして選択し、パレットまたはキー入力で割り当て

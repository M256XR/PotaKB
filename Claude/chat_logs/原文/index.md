# PotaKB セッション索引

## セッション別索引

### Session03 (2026-03-18) - 2026-03-18_Session03.txt
- L1: script.js キャリブレーション関数群実装（calibrateDevice/calibLoop/calibApply等）
- L50: keyboard-layout.json（KLE）解析・Configuratorキービジュアルを実物レイアウトに修正
- L120: FW書き込み後フリーズ → MCP23017 I2Cバスロックアップ診断・initMCP()にリカバリ追加
- L200: キャリブ中カーソルが動く → CALIB_START/END FWコマンド追加・updateStick()フラグ対応
- L250: キャリブ後キー無反応 → calibApply()がsaveToDevice()でキーマップ上書きしてた → config保存のみに修正

### Session02 (2026-03-18) - (要約のみ、原文なし)
- 実機テスト実施: スティック上下反転・USB起動しない・全キー無反応バグ修正
- 基本動作確認済み: キー入力・スティック（USB/BT両対応）

### Session01 (2026-03-18) - 2026-03-18_Session01.txt
- L1: 既存FWの問題分析（KC_TRNS=KC_NO バグ、スティック逆方向、ガクガク原因特定）
- L30: FW v2.0設計・仕様書(SPEC.md)作成
- L80: ハードウェア確認・PCBミス（X/Y逆配線）確認
- L100: FW v2.0フルリライト実施（keymap.h + PotaKB_fw.ino）
- L200: Configurator v2.0フルリライト（index.html / style.css / script.js）
- L280: キー入力キャプチャ機能追加（EVENT_CODE_TO_HID マッピング）
- L310: GitHub push・README修正・セッション管理システム導入

---

## トピック別索引

### スティック
- X/Y逆配線（PCBミス）確認・補正方針: S01 L80
- deltaTime方式速度計算: S01 L100
- EMAスムージング採用: S01 L100
- キャリブレーション機能（Configuratorモーダル + CALIB_START/END）: S03 L1
- stick_range_x/y（Config v2, 34バイト）: S03 L1
- I2Cバスロックアップリカバリ: S03 L120

### キーマップ・レイヤー
- KC_TRNS=0xFFFF / KC_NO=0x0000 バグ修正: S01 L1
- MO(n)方式レイヤーキー採用: S01 L100

### Bluetooth
- BT1/BT2/BT3+USB 4モード設計: S01 L30
- BLE接続間隔 7.5ms設定: S01 L100

### Configurator
- v2.0フルリライト: S01 L200
- キー入力キャプチャ機能: S01 L280
- Config 30バイト packed構造体: S01 L200
- Config 34バイト v2（stick_range_x/y追加）: S03 L1
- キービジュアル実物レイアウト修正（KLEから）: S03 L50
- キャリブレーションモーダルUI: S03 L1
- USB切断イベント検知: S03 L1

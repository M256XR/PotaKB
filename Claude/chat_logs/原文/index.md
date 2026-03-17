# PotaKB セッション索引

## セッション別索引

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
- キャリブレーション機能: S01 L100

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

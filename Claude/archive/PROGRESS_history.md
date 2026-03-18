# PROGRESS 決定事項ログ アーカイブ


### 2026-03-18（session04）
- キャリブレーション画面のスティック上下逆バグ修正（Configurator）
  - `calibDrawCanvas` でY軸を反転: `py = ((1023-y)/1023)*H`、範囲ボックスも同様
  - 原因: FWでは「Y値大＝上方向」だがキャンバスはY=0が上なので逆になっていた
- 電源OFF状態でUSB接続時に赤ランプ点灯＋スリープ入りするバグ修正（FW）
  - `setup()` の冒頭（`initHardware()`前）で POWER_SW_PIN を確認追加
  - HIGH（OFF状態）なら即 `NRF_POWER->SYSTEMOFF = 1` → LED点灯前に終了
  - USB接続時はSYSTEMOFFがリセットを引き起こすが setup()先頭から再実行されるためLED不点灯

### 2026-03-18（session03）
- スティックキャリブレーション機能を実装（Configurator UI + FWコマンド）
  - Config構造体に stick_range_x/y 追加（30→34バイト、CONFIG_VERSION=2）
  - Configuratorにキャリブレーションモーダル追加（3フェーズ: 中央記録→範囲計測→結果適用）
  - FW: `READ_STICK` / `CALIB_START` / `CALIB_END` シリアルコマンド追加
  - キャリブ中カーソル動く問題 → `CALIB_START` でスティック無効化
  - キャリブ後キー無反応 → `calibApply()` がキーマップも上書きしてた → config のみ保存に修正
- `keyboard-layout.json`（KLE）を元にConfigurator のキービジュアルを実物レイアウトに修正
  - Z行：左2u空白（スティックスペース）+ 11キー + LShift 1.5u
  - 底面：左6u空白 + Space 2.5u + 5キー1u
  - マウスボタン行：Btn1(1.5u) ... 12u gap ... Btn2(1.5u)
- MCP23017 I2Cバスロックアップ対策を `initMCP()` に追加（9クロックリカバリ）
- Configurator: USB切断イベント検知追加
- FW: I2Cリカバリ後もMCPへの書き込みハングでフリーズすることを確認
  - 原因: XIAOリセット時にMCPがI2Cトランザクション中断で詰まる場合あり
  - 対処: XIAOリセットボタンで復帰可能（電源断でも可）

### 2026-03-18（session02）
- 実機テスト実施、以下のバグを発見・修正
  - スティック上下反転 → `acc_mouse_y -= vy * dt` に修正
  - USB起動しない → 起動時USB優先ロジックに修正、USB抜き時saveMode削除
  - 全キー無反応 → 旧FWのkeymap.bin（256B）がフォーマット違いで読まれていた → サイズ・全ゼロチェックで自動削除
  - scanMatrix一括操作（writeGPIOAB/readGPIOAB）動作確認済み（旧FWと同等）
- 基本動作確認済み：キー入力・スティック（USB/BT両対応）

### 2026-03-18（session01）
- FW v2.0 フルリライト実施
  - KC_TRNS=0xFFFF / KC_NO=0x0000 に修正
  - マウス速度をvelocity×deltaTime方式に変更（BT/USB共通）
  - EMAスムージング採用（α=0.4デフォルト）
  - スティックA2→論理Y、A3→論理X（PCBミス補正を明示化）
  - BT1/BT2/BT3+USB 4モード切替実装
  - MO(n)方式のレイヤー操作キー採用
  - Config構造体を新設計（30バイト packed）
- Configurator v2.0 フルリライト
  - ダークテーマ、2カラムレイアウト
  - 物理キーボード入力でキーコード割り当て機能追加
  - BLE + USB Serial 両対応
  - DataViewで30バイトConfig構造体を正確に変換
- セッション管理システム導入（PortaRe0から移植）
- D:\Projects\PotaKB にリポジトリをクローン

# PotaKB 進捗管理

## 現在のフェーズ

**v2.0 実機テスト中**

## 現在の作業箇所

キャリブレーション機能を実装・バグ修正済み（FW書き込み待ち）。次回はキャリブ動作確認→残りテスト項目へ。

## フェーズ完了状況

| フェーズ | 状態 | 内容 |
|---------|------|------|
| ハードウェア設計 | ✅ 完了 | PCB・回路図・3Dモデル |
| FW v1.x | ✅ 完了（旧） | `old_fw/` に保管 |
| FW v2.0 リライト | ✅ 完了 | バグ修正・アーキテクチャ刷新 |
| Configurator v2.0 | ✅ 完了 | フルリライト |
| 実機テスト | 🔄 進行中 | 基本動作確認済み |
| BT複数台ボンド管理 | 🔄 未完全 | 将来拡張ポイント |

## 未解決・要テスト項目

- [x] マウス操作のガクガク修正が実機で効いているか確認（deltaTime方式）← BT接続で動作確認済み
- [x] スティック逆方向バグが解消されているか確認 ← 上下反転を実機で発見・修正済み
- [ ] KC_TRNSレイヤー透過が正しく動作するか確認
- [ ] BT1/BT2/BT3モード切替の動作確認
- [ ] Configurator: BLE接続・キーマップ読み書き確認
- [ ] Configurator: USB Serial接続・コマンド確認
- [ ] Configurator: キー入力キャプチャ機能の動作確認
- [ ] キャリブレーション機能の動作確認（実装済み・未テスト）

## v2.0 主要変更点（参考）

| 修正項目 | 旧 | 新 |
|---------|----|----|
| KC_TRNS vs KC_NO | 同じ0x0000（バグ） | KC_TRNS=0xFFFF、KC_NO=0x0000 |
| マウス速度 | 固定5倍補正 | velocity × deltaTime |
| スムージング | リングバッファ(SMOOTHING_SAMPLES=2) | EMA(alpha=0.4) |
| スティックX/Y | コード内で混乱 | A2→論理Y、A3→論理Xを明示 |
| BTモード | 1台のみ | BT1/BT2/BT3+USB 4モード |
| モード保存 | なし | mode.binにFlash保存 |
| レイヤーキー | L_LOWER固定値 | MO(n)方式 |

---

## 直近の決定事項ログ

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

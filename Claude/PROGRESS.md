# PotaKB 進捗管理

## 現在のフェーズ

**v2.0 実機テスト中**

## 現在の作業箇所

BLE多スロット・USB hi-resスクロール・各種バグ修正完了。GitHub pushおよびGitHub Pages更新済み。主要機能は実機動作確認済み。

## フェーズ完了状況

| フェーズ | 状態 | 内容 |
|---------|------|------|
| ハードウェア設計 | ✅ 完了 | PCB・回路図・3Dモデル |
| FW v1.x | ✅ 完了（旧） | `old_fw/` に保管 |
| FW v2.0 リライト | ✅ 完了 | バグ修正・アーキテクチャ刷新 |
| Configurator v2.0 | ✅ 完了 | フルリライト |
| 実機テスト | 🔄 進行中 | 基本動作確認済み |
| BT複数台ボンド管理 | ✅ 完了 | スロット別アドレス・ボンドDB分離で解決 |

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

### 2026-03-18（session05）
- BLEスロット別ボンドDB分離（bonding.cpp 改造）
  - 同一PCで複数スロットを使うとボンドファイルが同一MACで上書きされLTKミスマッチ → HID無応答が根本原因
  - `bond_set_slot(slot)` でアクティブディレクトリを `/adafruit/bond_prph_1/` 等に切り替え
  - `startAdvertising()` 呼び出し前に `bond_set_slot(btSlot)` を追加
- USB自動切替: エッジ検出方式に変更
  - `prev_usb_mounted` フラグで立ち上がりエッジのみ検出 → USB刺しながらBTモードに手動切替してもループしない
- スリープ復帰バグ修正（FW: wakeUp()）
  - スリープ中に `lastStickUpdateTime` が凍結→巨大dt→acc_mouseが±127に即到達→操作不能
  - `wakeUp()` で `lastStickUpdateTime = millis()` + 全accumulator/smoothingをゼロリセット
  - `prev_kb_mod/keys/mouse_btn` も 0xFF にリセット（ゴースト入力防止）
- Configurator 修正
  - BLEフィルタ: `{ name: 'PotaKB' }` → `{ namePrefix: 'PotaKB' }`（PotaKB-1/2/3対応）
  - USB Config読み込みサイズ: 34→35バイト（DataView RangeError修正）
  - BLE/USB接続時にバッテリー残量を自動取得
- GitHub push・README更新・GitHub Pages(M256XR.github.io/poke)のConfiguratorを最新版に更新

### 2026-03-18（session04・続き3）
- スクロールモメンタム（慣性）実装（FW）
  - スティックがデッドゾーン内のとき、EMAで即ゼロではなく半減期200msの指数減衰
  - スティックが動いているときはEMAで速度追従（変更なし）
  - 定数: 0.0035/ms = ln(2)/200ms
  - 効果: スティックを離した後もスクロールがなめらかに減速
- スクロール初期速度を 0.06 → 0.02 tick/ms に変更（速すぎた）
- スクロール反転が効かない件: コードは正しい。Configuratorで「デバイスへ保存する」が必要

### 2026-03-18（session04・続き2）
- Arduino IDEなしの書き込み環境を `tools/` に構築
  - `setup.ps1`: arduino-cli DL + Adafruit nRF52 コア + ライブラリ一括セットアップ（tools/内に閉じ込め）
  - `build.ps1`: コンパイル（FQBN: adafruit:nrf52:Seeed_XIAO_nRF52840）
  - `flash.ps1`: シリアルDFU書き込み（COMポート自動検出 or -Port 指定）
  - `flash_uf2.ps1`: UF2ブートローダー経由書き込み（XIAO_BOOTドライブ自動検出）

### 2026-03-18（session04・続き）
- スクロール方向反転設定を追加（Config + Configurator）
  - Config struct に `scroll_invert`(uint8) 追加 → 35バイト、CONFIG_VERSION=3
  - magic オフセット 30→31 に移動
  - Configurator「スクロール方向を反転」チェックボックス追加
- スクロールスムージング改善（FW）
  - `smooth_scroll_v / smooth_scroll_h` グローバル変数を追加
  - スクロール速度にEMA適用（`stick_ema_alpha` を流用）
  - マウスモード中はスクロール速度変数をリセット（モード切替時バースト防止）
  - 効果: 速度変化が徐々になるため整数HIDステップの間隔が均等化→カクつき軽減

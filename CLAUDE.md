# Claude Code 設定

## プロジェクト情報
- **名前**: Handy Keyboard Firmware
- **種類**: ESP32-P4 ファームウェア（組み込み開発）
- **言語**: Rust（esp-idf-hal + LVGL Rust binding）
- **ビルドシステム**: ESP-IDF v5.5.2 + Cargo

## コーディング規約

### 統合アプリ (`03.firmware/04.integratedAPP/`)
- **モジュールは適度に分割**してシンプルでメンテナンスしやすい設計
- **依存関係を整理**して意図のわかりやすい構造に
- **Rust の慣習に従う**: `cargo fmt`, `cargo clippy` を活用
- テストコードは**シンプルさを最優先**

### 命名規則
- **関数・変数**: `snake_case` （例: `keyboard_init`, `handle_touch_event`）
- **型・トレイト**: `PascalCase` （例: `KeyboardState`, `TouchHandler`）
- **定数**: `UPPER_SNAKE_CASE` （例: `MAX_BUFFER_SIZE`）
- **モジュール**: `snake_case` （例: `keyboard`, `touch_handler`）

### コメント
- **必須**: 複雑なロジック、ハードウェア依存部分、ワークアラウンド
- **不要**: 自明な処理

## ビルド・実行

### Rust + ESP-IDF ビルド
`.claude/skills/esp-idf/` が自動適用されます。

```bash
# 環境変数設定（初回のみ）
export IDF_ID="esp-idf-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

# 統合アプリのビルド（Cargo経由）
cd 03.firmware/04.integratedAPP
cargo build --release

# フラッシュ・モニタ（esp-idf経由、またはcargo-espflash使用）
cargo espflash flash --release --monitor
```

### 使用クレート
- **esp-idf-hal**: ESP32 ハードウェア抽象化層
- **LVGL Rust binding**: GUI フレームワーク（lvgl-rs など）
- **esp-idf-svc**: WiFi/BLE サービス
- **embedded-hal**: 汎用組み込みトレイト

## 重要な制約
- **既存ファイルの保護**: 既存の実装やテストを勝手に変更しない
- **過剰な抽象化の禁止**: 必要最小限の実装に留める
- **非破壊マージ**: 設定ファイルは既存設定を尊重

## ドキュメント
- `01.docs/hardware/`: ハードウェア仕様・ピン配置
- `01.docs/development/`: 開発ガイド・アーキテクチャ
- `README.md`: プロジェクト概要

## 既知の問題
- スピーカーテスト (`03.speaker_check`) で高周波数帯のデジタルノイズ
- 統合アプリで修正予定

## 品質保護
- `.claude/rules/test-quality.md`: テスト改ざん禁止
- `.claude/rules/implementation-quality.md`: 形骸化実装禁止

# Handy Keyboard Firmware (ESP32-P4)

## 概要
JC4880P443C-I-W (ESP32-P4 + ESP32-C6) を使用した多機能ハンディキーボード/コントローラのファームウェアプロジェクトです。
シンプルで保守性の高い設計を目指した個人ホビープロジェクトです。

## ハードウェア
- **Main MCU**: ESP32-P4 (High-performance, no radio)
- **Radio MCU**: ESP32-C6 (WiFi 6 + BLE 5.4, connected via SDIO)
- **Audio**: ES8311 Codec + Speaker
- **Display**: MIPI-DSI LCD + Touch (GT911)
- **Storage**: MicroSD Card
- **Sensors**: Camera (OV5640), RTC (DS3231)
- **Connectivity**: WiFi (via ESP-Hosted), BLE (via ESP-Hosted)

## 主要機能
- **BTキーボード**: フリック入力、半角英数入力
- **BTマウス**: タッチパネルをマウスとして使用
- **時計モード**: デジタル/アナログ表示、アラーム機能
- **WoL (Wake-on-LAN)**: PCの電源ON
- **各種設定**: WiFi設定、BT接続先管理、画像/音楽設定
- **MicroSD**: 画像・動画表示、音楽再生、USBマスストレージ

## 開発環境
- **ESP-IDF**: v5.5.2
- **Build System**: Ubuntu (bash + idf.py)
- **Language**: Rust (esp-idf-hal + LVGL Rust binding)
- **Version Control**: GitHub
- **Communication**: ESP-Hosted over SDIO (P4 ↔ C6)

## ディレクトリ構成
- `01.docs/`: ドキュメント
  - `hardware/`: ハードウェア仕様・ピン配置
- `.claude/skills/esp-idf/`: ESP-IDF スキル定義
- `03.firmware/`: ファームウェアソースコード
  - `03.speaker_check/`: スピーカー出力テスト
  - `04.integratedAPP/`: 統合アプリケーション (メイン開発、Rust)
- `04.reference/`: 参考資料 (データシート等)

## ビルド・実行

### 前提条件
1. ESP-IDF v5.5.2 がインストール済み
2. Rust + cargo がインストール済み
3. cargo-espflash がインストール済み（推奨）

### 環境設定
```bash
# ESP-IDF 環境のソース（セッションごとに必要）
. $HOME/esp/esp-idf/export.sh

# または ~/.bashrc に追記して自動化
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.bashrc
```

### ビルド方法（Rust統合アプリ）
```bash
# プロジェクトディレクトリに移動
cd 03.firmware/04.integratedAPP

# Cargoでビルド
cargo build --release

# フラッシュ・モニタ（cargo-espflash使用）
cargo espflash flash --release --monitor

# またはidf.py経由
idf.py flash monitor
```

### C/C++プロジェクト（スピーカーテスト）
```bash
# スピーカーテストのビルド
cd 03.firmware/03.speaker_check
idf.py build

# フラッシュ・モニタ
idf.py flash monitor
```

## コーディング規約（統合アプリ - Rust）
- **モジュールは適度に分割**してシンプルでメンテナンスしやすい設計
- **依存関係を整理**して意図のわかりやすい構造に
- **Rust の慣習に従う**: `cargo fmt`, `cargo clippy` を活用
- テストコードは**シンプルさを最優先**

## 既知の問題
### スピーカーテスト (`03.speaker_check`)
- 高周波数帯でデジタルノイズが発生
- 原因候補: バッファ不足、設定ミス、計算処理の遅延
- 統合アプリで修正予定

## 開発状況
- ✅ スピーカー出力動作確認
- ✅ ESP-Hosted over SDIO設定
- 🚧 統合アプリ実装中（ZEROから構築）

## 参考プロジェクト
- `keyboard_2025`: ESP32-P4 + C6 構成（BLEのみ）の参考実装

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
- **Build System**: Windows (PowerShell via `idf_run.ps1`)
- **Version Control**: GitHub
- **Communication**: ESP-Hosted over SDIO (P4 ↔ C6)

## ディレクトリ構成
- `01.docs/`: ドキュメント
  - `hardware/`: ハードウェア仕様・ピン配置
- `.agents/skills/esp-idf/`: ESP-IDFビルドスクリプト
- `03.firmware/`: ファームウェアソースコード
  - `03.speaker_check/`: スピーカー出力テスト
  - `04.integratedAPP/`: 統合アプリケーション (メイン開発)
- `04.reference/`: 参考資料 (データシート等)

## ビルド・実行

### 前提条件
1. ESP-IDF v5.5.2がインストール済み
2. 環境変数 `IDF_ID` が設定済み（例: `esp-idf-b29c58f93b4ca0f49cdfc4c3ef43b562`）
   - `idf-env config list` で確認可能

### ビルド方法
`.agents/skills/esp-idf/idf_run.ps1` を使用してビルドします。

```powershell
# 環境変数設定（初回のみ）
$env:IDF_ID = "esp-idf-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

# スピーカーテストのビルド
$env:IDF_PROJECT_PATH = "03.firmware/03.speaker_check"
.\.agents\skills\esp-idf\idf_run.ps1 build

# 統合アプリのビルド
$env:IDF_PROJECT_PATH = "03.firmware/04.integratedAPP"
.\.agents\skills\esp-idf\idf_run.ps1 build

# フラッシュ
.\.agents\skills\esp-idf\idf_run.ps1 flash monitor
```

## コーディング規約（統合アプリのみ）
- **.cファイルは300行程度**を目安として適度に分割
- **依存関係を整理**してシンプルでメンテナンスしやすい設計
- **意図のわかりやすい**実装を心がける
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

# Handy Keyboard Firmware (ESP32-P4)

## 概要
JC4880P443C-I-W (ESP32-P4 + ESP32-C6) を使用した多機能ハンディキーボード/コントローラのファームウェアプロジェクトです。
`keyboard_2025` プロジェクト (KE-OSアーキテクチャ) の設計思想を踏襲し、高い保守性と拡張性を目指します。

## ハードウェア
- **Main MCU**: ESP32-P4 (High-performance, no radio)
- **Radio MCU**: ESP32-C6 (WiFi 6 + BLE 5.4, connected via SDIO/UART)
- **Audio**: ES8311 Codec + Speaker
- **Display**: MIPI-DSI LCD + Touch (GT911)
- **Sensors**: Camera (OV5640), RTC (DS3231)

## 開発環境
- **ESP-IDF**: v5.5.2
- **Build System**: Windows (PowerShell via `idf_run.ps1`)

## ディレクトリ構成
- `01.docs/`: ドキュメント
- `03.firmware/`: ファームウェアソースコード
    - `01.switchbot_hub2_test/`: SwitchBot制御テスト
    - `03.speaker_check/`: スピーカー出力テスト
    - `04.integratedAPP/`:統合アプリケーション (Main)
- `04.reference/`: 参考資料 (データシート、旧GUIなど)

## ビルド・実行
`.agents/skills/esp-idf/idf_run.ps1` を使用してビルドします。

```powershell
# 例: スピーカーテストのビルド
.agents/skills/esp-idf/idf_run.ps1 -p 03.firmware/03.speaker_check build
```

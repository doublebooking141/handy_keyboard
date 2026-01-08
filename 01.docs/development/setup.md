# 開発環境セットアップ

## 必要なツール
- **ESP-IDF v5.5.2**
- **Git for Windows** (Git Bash)
- **PowerShell** (Windows標準)
- **Visual Studio Code** (推奨エディタ)

## ESP-IDFのインストール

### 1. ESP-IDF Windows Installerを使用
1. [ESP-IDF公式サイト](https://dl.espressif.com/dl/esp-idf/)から Windows Installer をダウンロード
2. インストーラーを実行し、ESP-IDF v5.5.2を選択
3. インストール完了後、`idf-env config list` で IDF_ID を確認

### 2. IDF_ID の取得
```powershell
# PowerShellで実行
idf-env config list
```

出力例:
```
esp-idf-b29c58f93b4ca0f49cdfc4c3ef43b562:
  IDF version: v5.5.2
  Path: C:\Espressif\frameworks\esp-idf-v5.5.2
  Python: C:\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe
```

この場合、IDF_IDは `esp-idf-b29c58f93b4ca0f49cdfc4c3ef43b562` です。

### 3. 環境変数の設定
```powershell
# PowerShellで設定（セッションごとに必要）
$env:IDF_ID = "esp-idf-b29c58f93b4ca0f49cdfc4c3ef43b562"

# 永続的に設定する場合（推奨）
[System.Environment]::SetEnvironmentVariable("IDF_ID", "esp-idf-b29c58f93b4ca0f49cdfc4c3ef43b562", "User")
```

## プロジェクトのクローン
```bash
git clone <repository-url> handy_keyboard
cd handy_keyboard
```

## ビルド

### スピーカーテストのビルド
```powershell
$env:IDF_PROJECT_PATH = "03.firmware/03.speaker_check"
.\.agents\skills\esp-idf\idf_run.ps1 build
```

### 統合アプリのビルド
```powershell
$env:IDF_PROJECT_PATH = "03.firmware/04.integratedAPP"
.\.agents\skills\esp-idf\idf_run.ps1 build
```

## フラッシュ & モニター
```powershell
# フラッシュ
.\.agents\skills\esp-idf\idf_run.ps1 flash

# シリアルモニター
.\.agents\skills\esp-idf\idf_run.ps1 monitor

# フラッシュ & モニター
.\.agents\skills\esp-idf\idf_run.ps1 flash monitor
```

## ESP32-C6 への ESP-Hosted Slave のフラッシュ

ESP32-C6側にはESP-Hosted MCU Slaveファームウェアが必要です。

### 1. ESP-Hosted リポジトリのクローン
```bash
git clone --recursive https://github.com/espressif/esp-hosted.git
cd esp-hosted/esp_hosted_fg/esp/esp_driver/network_adapter
```

### 2. C6向けにビルド
```bash
idf.py set-target esp32c6
idf.py menuconfig
# Example Configuration > Transport layer > SDIO を選択
idf.py build
```

### 3. フラッシュ
```bash
idf.py -p COMx flash
```

## トラブルシューティング

### ビルドエラー: "IDF_ID environment variable not set"
- 環境変数 `IDF_ID` が設定されていません
- `idf-env config list` で IDF_ID を確認し、設定してください

### ビルドエラー: "Unable to find ESP-IDF at: ..."
- IDF_ID が間違っているか、ESP-IDFがインストールされていません
- `idf-env config list` で正しいパスを確認してください

### Git Bashからビルドする場合
```bash
# PowerShell経由で実行
powershell.exe -Command '$env:IDF_ID="esp-idf-xxx"; $env:IDF_CMD="build"; & ".\.agents\skills\esp-idf\idf_run.ps1"'
```

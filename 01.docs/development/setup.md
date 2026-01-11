# 開発環境セットアップ（Ubuntu）

## 必要なツール
- **ESP-IDF v5.5.2**
- **Rust + Cargo** (rustup経由推奨)
- **cargo-espflash** (フラッシュツール)
- **Git**
- **Visual Studio Code** (推奨エディタ)

## Rustのインストール

```bash
# rustupを使用してRustをインストール
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# 環境を再読み込み
source $HOME/.cargo/env

# バージョン確認
rustc --version
cargo --version
```

## cargo-espflashのインストール

```bash
cargo install cargo-espflash
```

## ESP-IDFのインストール

### 1. 必要なパッケージのインストール
```bash
sudo apt-get update
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv \
  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

### 2. ESP-IDFのクローン
```bash
mkdir -p $HOME/esp
cd $HOME/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.5.2
git submodule update --init --recursive
```

### 3. ESP-IDFツールのインストール
```bash
./install.sh esp32,esp32p4,esp32c6
```

### 4. 環境のセットアップ
```bash
# 一時的にセットアップ（セッションごとに必要）
. $HOME/esp/esp-idf/export.sh

# または ~/.bashrc に alias を追加
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.bashrc
source ~/.bashrc

# 以降は get_idf コマンドで環境をソース可能
```

## プロジェクトのクローン
```bash
git clone <repository-url> handy_keyboard
cd handy_keyboard
```

## ビルド

### ESP-IDF環境のソース
```bash
# セッションごとに必要
. $HOME/esp/esp-idf/export.sh

# または alias を使用（上記で設定した場合）
get_idf
```

### 統合アプリのビルド（Rust）
```bash
cd 03.firmware/04.integratedAPP
cargo build --release
```

### スピーカーテストのビルド（C）
```bash
cd 03.firmware/03.speaker_check
idf.py build
```

## フラッシュ & モニター

### 統合アプリ（Rust）
```bash
cd 03.firmware/04.integratedAPP

# cargo-espflash使用（推奨）
cargo espflash flash --release --monitor

# またはidf.py経由
idf.py flash monitor
```

### スピーカーテスト（C）
```bash
cd 03.firmware/03.speaker_check
idf.py flash monitor
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
# シリアルポートを確認（通常は /dev/ttyUSB0 または /dev/ttyACM0）
ls /dev/tty*

# フラッシュ
idf.py -p /dev/ttyUSB0 flash
```

## トラブルシューティング

### ビルドエラー: "IDF_PATH environment variable not set"
- ESP-IDF環境がソースされていません
- `. $HOME/esp/esp-idf/export.sh` を実行してください

### ビルドエラー: "Unable to find ESP-IDF"
- ESP-IDFがインストールされていないか、パスが間違っています
- `$HOME/esp/esp-idf/` にインストールされているか確認してください

### シリアルポートの権限エラー
```bash
# ユーザーをdialoutグループに追加
sudo usermod -a -G dialout $USER

# ログアウト・ログインして反映
```

### cargo-espflashが見つからない
```bash
# cargo-espflashのインストール
cargo install cargo-espflash

# PATHの確認
which cargo-espflash
```

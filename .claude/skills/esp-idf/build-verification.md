# ESP-IDF Build Verification

ビルドによるコード検証手順（Ubuntu環境）。

## 環境準備

ESP-IDF環境をソースします：
```bash
. $HOME/esp/esp-idf/export.sh
```

## 通常ビルド

**統合アプリ（Rust）:**
```bash
cd 03.firmware/04.integratedAPP
cargo build --release
```

**スピーカーテスト（C）:**
```bash
cd 03.firmware/03.speaker_check
idf.py build
```

## フルクリーンビルド

**以下のファイルを変更した場合に実行:**
- `sdkconfig`
- `sdkconfig.defaults`
- `Kconfig`
- `Kconfig.projbuild`
- `CMakeLists.txt` (プロジェクトルート)
- `partitions.csv`
- `Cargo.toml` (Rustプロジェクト)

**手順（統合アプリ - Rustの場合）:**

1. クリーンビルド
   ```bash
   cd 03.firmware/04.integratedAPP
   cargo clean
   cargo build --release
   ```

**手順（スピーカーテスト - Cの場合）:**

1. sdkconfig削除
   ```bash
   cd 03.firmware/03.speaker_check
   rm -f sdkconfig
   ```

2. フルクリーン実行
   ```bash
   idf.py fullclean
   ```

3. ビルド実行
   ```bash
   idf.py build
   ```

## ビルド結果の判定

- **Exit code 0**: 成功（変更は安全）
- **Exit code 非ゼロ**: 失敗（変更を元に戻す必要あり）

ログファイルは `build/log/build_YYYYMMDD_HHMMSS.log` に保存される。

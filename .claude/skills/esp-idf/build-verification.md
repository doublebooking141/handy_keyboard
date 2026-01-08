# ESP-IDF Build Verification

ビルドによるコード検証手順。

## 環境変数

IDF_IDは `idf-env config list` で確認できます。

## 通常ビルド

**統合アプリ:**
```bash
pwsh -NoProfile -ExecutionPolicy Bypass -Command '$env:IDF_ID="<your-idf-id>"; $env:IDF_PROJECT_PATH="C:\Users\doubl\Desktop\ai_test\handy_keyboard\03.firmware\04.integratedAPP"; $env:IDF_CMD="build"; & "C:\Users\doubl\Desktop\ai_test\handy_keyboard\.claude\skills\esp-idf\idf_run.ps1"'
```

**スピーカーテスト:**
```bash
pwsh -NoProfile -ExecutionPolicy Bypass -Command '$env:IDF_ID="<your-idf-id>"; $env:IDF_PROJECT_PATH="C:\Users\doubl\Desktop\ai_test\handy_keyboard\03.firmware\03.speaker_check"; $env:IDF_CMD="build"; & "C:\Users\doubl\Desktop\ai_test\handy_keyboard\.claude\skills\esp-idf\idf_run.ps1"'
```

## フルクリーンビルド

**以下のファイルを変更した場合に実行:**
- `sdkconfig`
- `sdkconfig.defaults`
- `Kconfig`
- `Kconfig.projbuild`
- `CMakeLists.txt` (プロジェクトルート)
- `partitions.csv`

**手順（統合アプリの場合）:**

1. sdkconfig削除（defaultsから再生成させる）
   ```bash
   rm -f "C:\Users\doubl\Desktop\ai_test\handy_keyboard\03.firmware\04.integratedAPP\sdkconfig"
   ```

2. フルクリーン実行
   ```bash
   pwsh -NoProfile -ExecutionPolicy Bypass -Command '$env:IDF_ID="<your-idf-id>"; $env:IDF_PROJECT_PATH="C:\Users\doubl\Desktop\ai_test\handy_keyboard\03.firmware\04.integratedAPP"; $env:IDF_CMD="fullclean"; & "C:\Users\doubl\Desktop\ai_test\handy_keyboard\.claude\skills\esp-idf\idf_run.ps1"'
   ```

3. ビルド実行（上記の通常ビルドコマンド）

**手順（スピーカーテストの場合）:**

1. sdkconfig削除
   ```bash
   rm -f "C:\Users\doubl\Desktop\ai_test\handy_keyboard\03.firmware\03.speaker_check\sdkconfig"
   ```

2. フルクリーン実行
   ```bash
   pwsh -NoProfile -ExecutionPolicy Bypass -Command '$env:IDF_ID="<your-idf-id>"; $env:IDF_PROJECT_PATH="C:\Users\doubl\Desktop\ai_test\handy_keyboard\03.firmware\03.speaker_check"; $env:IDF_CMD="fullclean"; & "C:\Users\doubl\Desktop\ai_test\handy_keyboard\.claude\skills\esp-idf\idf_run.ps1"'
   ```

3. ビルド実行（上記の通常ビルドコマンド）

## ビルド結果の判定

- **Exit code 0**: 成功（変更は安全）
- **Exit code 非ゼロ**: 失敗（変更を元に戻す必要あり）

ログファイルは `build/log/build_YYYYMMDD_HHMMSS.log` に保存される。

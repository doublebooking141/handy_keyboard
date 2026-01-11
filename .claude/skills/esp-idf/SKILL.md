---
name: esp-idf
description: ESP-IDF開発プロジェクトで自動適用。Ubuntu環境でidf.pyコマンド（build, flash, monitor等）を直接実行可能。
---

# ESP-IDF Development Skill

ESP-IDF (Espressif IoT Development Framework) を使用した組み込み開発プロジェクト向けスキル（Ubuntu環境）。

## 環境設定

ESP-IDF環境をソースすることで、idf.pyコマンドが使用可能になります：

```bash
# ESP-IDF環境のソース（セッションごとに必要）
. $HOME/esp/esp-idf/export.sh

# または ~/.bashrc に alias を追加
echo 'alias get_idf=". $HOME/esp/esp-idf/export.sh"' >> ~/.bashrc
```

## idf.pyコマンド実行

プロジェクトディレクトリに移動して直接実行します：

```bash
# 統合アプリ（Rust）のビルド
cd 03.firmware/04.integratedAPP
cargo build --release
cargo espflash flash --release --monitor

# スピーカーテスト（C）のビルド
cd 03.firmware/03.speaker_check
idf.py build
idf.py flash monitor
```

## 実行可能なコマンド

### 通常実行（Bashツール経由）
- `--version` - バージョン確認
- `build` - ビルド実行（タイムアウト: 600秒）
- `fullclean` - ビルドキャッシュのクリア
- `flash` - フラッシュ書き込み（タイムアウト: 120秒）
- `monitor` - シリアルモニター（タイムアウト: 60秒、ログ保存）
- `flash monitor` - フラッシュ後モニター起動（推奨、タイムアウト: 180秒）
- `size` - バイナリサイズ確認
- `app-flash` - アプリのみフラッシュ（高速）

### タイムアウト推奨値
- **起動ログ確認**: 30000ms (30秒)
- **動作テスト**: 60000-120000ms (1-2分)
- **長時間監視**: 必要に応じて延長

### 設定管理

**sdkconfig編集（推奨方法）:**
- `sdkconfig.defaults`ファイルを直接編集
- 変更後に`idf.py build`で自動反映

**menuconfig（対話的UI）:**
- ncursesベースのため、対話的操作が必要
- 設定確認のみなら`sdkconfig`ファイルを読み取り可能
- 設定変更が必要な場合はユーザーに依頼

## 実行可能な操作

- ソースコード編集 (`.c`, `.h`, `.cpp`, `.hpp`)
- `CMakeLists.txt` / `Kconfig` / `sdkconfig` の編集
- コンパイルエラーの分析
- すべての`idf.py`コマンド（flash, monitorを含む）の実行
- シリアルモニター出力のログ解析

## monitor出力の取得方法

monitor実行時、出力はログファイルに保存されます:
- `build/log/build_YYYYMMDD_HHMMSS.log`

タイムアウト後、このファイルを読み取って動作確認します。

## 設定管理

**設定確認:**
- `sdkconfig`ファイルを読み取る

**設定変更（推奨）:**
- `sdkconfig.defaults`を編集後、`idf.py build`で反映

**対話的menuconfig:**
- ncursesベースのため対話的操作が必要
- 設定変更が必要な場合はユーザーに依頼

## ロギングポリシー

テスト完了後のログ整理:

**重要なログ（状態変化・エラー）:**
- `ESP_LOGI` → `ESP_LOGD` に変更（DEBUGレベルに下げる）
- 例: 状態変更、設定変更、接続状態

**さほど重要でないログ:**
- ログ出力自体を削除
- 例: 詳細な初期化ステップ、一時的な確認ログ

**常にINFOレベルで残すべきログ:**
- 初期化成功/失敗（ESP_ERROR_CHECK前のログ）
- システムモード変更
- 致命的エラー（ESP_LOGE）

## ハードウェアテスト指示書

実機での操作・確認が必要な場合:

**ファイル作成:**
- ディレクトリ: `01.docs/firmware/system_tests/`
- ファイル名: `TEST_<機能名>.md` (例: `TEST_audio_output.md`)
- 目的: ユーザーに実行してほしい手順を明確に記載

**記載内容:**
- テスト対象（変更内容の説明）
- デバイス設定
- 実行コマンド（flash/monitor、ポート指定）
- 期待動作とログ
- 結果記録欄

**命名規則:**
- `TEST_XXX_<feature_name>.md` - 連番 + 機能名
- 例: `TEST_001_speaker_output.md`, `TEST_002_display_init.md`

## 関連ドキュメント

- `build-verification.md` - ビルド検証手順とフルクリーン条件

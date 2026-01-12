# Plans

## 🎯 プロジェクト: Handy Keyboard Firmware

### 概要
- **目的**: ESP32-P4ベースの多機能ハンディキーボード/コントローラ
- **言語**: **C/C++ + ESP-IDF**（Rust試行後、切り替え）
- **フレームワーク**: ESP-IDF v5.5.2
- **GUI**: LVGL（C版）

### 技術スタック
- **メインMCU**: ESP32-P4（RISC-V dual-core 400MHz）
- **無線MCU**: ESP32-C6（SDIO接続）
- **言語**: C/C++
- **フレームワーク**: ESP-IDF v5.5.2
- **ビルドシステム**: CMake + idf.py
- **GUI**: LVGL v8/v9（C版）

---

## 🔄 方針転換：Rust → C/C++ + ESP-IDF

**決定日**: 2026年1月12日

### 理由
1. **ESP32-P4 の Rust サポートが実験的段階**
   - ESP-IDF v5.3 では動作するが、v5.5.2 は esp-idf-hal v0.45.2 が未対応
   - 8つのコンパイルエラー（`Core1` 認識不可、`hys_ctrl_mode` フィールド欠落など）

2. **ディスプレイ・カメラのサポートが必要**
   - LVGL、MIPI-DSI、MIPI-CSI は ESP-IDF (C) で完全サポート
   - Rust エコシステムでは周辺機器サポートが不十分

3. **開発速度の優先**
   - 確実に動作する C/C++ + ESP-IDF で進める

### Rust 試行の成果
- **04.integratedAPP/README.md** に詳細な試行記録を保存
- ESP-IDF v5.3 での成功構成を記録（将来の参考用）
- 参考資料・問題解決策をドキュメント化

---

## 現在のタスク

### ✅ フェーズ0: Rust試行（完了 → 方針転換）

**試行期間**: 2026年1月7日〜12日

**成果**:
- ESP-IDF v5.3 ではビルド成功（`CRATE_CC_NO_DEFAULTS=1` が決定打）
- ESP-IDF v5.5.2 は esp-idf-hal v0.45.2 が未対応と判明
- 詳細な試行記録・解決策を **`03.firmware/04.integratedAPP/README.md`** に保存

**参考資料**:
- [esp-rs/esp-idf-sys Issue #176](https://github.com/esp-rs/esp-idf-sys/issues/176) - `CRATE_CC_NO_DEFAULTS=1` 回避策
- [Rust on ESP32-P4 Tier 2 Promotion](https://github.com/rust-lang/compiler-team/issues/864)

---

### 🟡 cc:TODO フェーズ1: C/C++ + ESP-IDF セットアップ

- [ ] ESP-IDF v5.5.2 環境確認
- [ ] 既存の 01.mainMCU コードの確認・整理
- [ ] プロジェクト構成の決定（統合 or 分離）
- [ ] CMakeLists.txt の整備
- [ ] sdkconfig の設定
- [ ] ビルド・フラッシュの動作確認

### 🟢 cc:TODO フェーズ2: 基本動作確認

- [ ] フラッシュ書き込み成功確認
- [ ] シリアルモニターでログ出力確認
- [ ] ESP32-P4起動確認

### 🟢 cc:TODO フェーズ3: LVGL統合（C版）

- [ ] LVGL v8/v9 の選定
- [ ] components/lvgl ディレクトリ作成
- [ ] lv_conf.h 設定ファイル作成
- [ ] MIPI-DSI ディスプレイドライバ統合
- [ ] LVGL初期化コード追加
- [ ] 簡単な UI テスト（Hello World 表示）

### 🔵 cc:TODO フェーズ4: I2C周辺デバイス初期化

- [ ] I2Cバス初期化（GPIO7/GPIO8）
- [ ] DS3231M RTC通信確認（アドレス 0x68）
- [ ] ES8311 Audio Codec通信確認（アドレス 0x18）
- [ ] GT911 Touch通信確認（アドレス 0x5D）
- [ ] I2Cアドレススキャン実装

## 完了したタスク
- ✅ ハーネス初期セットアップ（AGENTS.md、CLAUDE.md、Plans.md 生成）
- ✅ Rust試行・検証（ESP-IDF v5.3で成功、v5.5.2は未対応と判明）
- ✅ 方針決定：C/C++ + ESP-IDF で開発継続
- ✅ ハードウェアドキュメント整備（DS3231M RTC追加記録）
- ✅ Windows→Ubuntu環境移行

---

## 使い方

### タスクの追加
```markdown
### cc:TODO タスク名
- [ ] サブタスク1
- [ ] サブタスク2
```

### タスクの開始
```markdown
### cc:WIP タスク名
- [x] 完了したサブタスク
- [ ] 作業中のサブタスク
```

### タスクの完了
マーカーを削除して「完了したタスク」セクションに移動:
```markdown
## 完了したタスク
- ✅ タスク名
```

### ブロック中のタスク
```markdown
### cc:blocked タスク名
依存: #123 の完了待ち
```

---

## コマンド
- `/plan-with-agent <内容>`: 新しい計画を作成
- `/work`: このファイルのタスクを実行
- `/sync-status`: 進捗確認・更新

# Integrated Application

## 概要
ESP32-P4ベースのHandy Keyboard統合ファームウェアです。

**現在の実装**: **C/C++ + ESP-IDF v5.5.2**
**過去の試行**: Rust + ESP-IDF（2026年1月時点で互換性問題により中止）

---

## ⚠️ 重要：ビルド環境

### 必須：ESP-IDF v5.5.2を使用

```bash
# ❌ 間違い（v6.1が起動する）
. $HOME/esp/esp-idf/export.sh

# ✅ 正しい（v5.5.2を明示的に指定）
. $HOME/esp/esp-idf-v5.5.2/export.sh
```

**理由**:
- プロジェクトはESP-IDF v5.5.2向けに設定（`dependencies.lock`）
- v6.1では異なるツールチェーン（esp-15.2.0）が使用され、互換性なし
- v5.5.2では esp-14.2.0 ツールチェーンが必要

### チップリビジョン設定

実機がESP32-P4 revision v1.0の場合、`sdkconfig.defaults`に以下が設定されています：

```ini
CONFIG_ESP32P4_REV_MIN_100=y
CONFIG_ESP32P4_REV_MIN_FULL=100
CONFIG_ESP32P4_REV_MAX_FULL=199
```

---

## ビルド手順

```bash
# 1. ESP-IDF v5.5.2環境をソース
. $HOME/esp/esp-idf-v5.5.2/export.sh

# 2. プロジェクトディレクトリへ移動
cd 03.firmware/04.integratedAPP

# 3. ビルド
idf.py build

# 4. フラッシュ & モニター
idf.py -p /dev/ttyACM0 flash monitor
```

**初回ビルド時**: `dependencies.lock`に従ってManaged Componentsが自動ダウンロードされます。

---

## Rust試行の記録（参考）

このディレクトリは元々、ESP32-P4 ファームウェアを **Rust + ESP-IDF** で実装する試みを行った場所です。

**結論**: 2026年1月時点では、ESP32-P4 + Rust + ESP-IDF v5.5.2 の組み合わせは困難であり、**C/C++ + ESP-IDF での開発に戻す**ことを決定しました。

## 試行内容（2026年1月12日）

### 目標
- ESP32-P4 向けの Rust ファームウェアを構築
- esp-idf-hal + esp-idf-svc を使用
- ESP-IDF v5.5.2 で動作させる

### 技術スタック（試行時）
- **言語**: Rust 1.75+
- **ターゲット**: riscv32imafc-esp-espidf
- **フレームワーク**: ESP-IDF v5.5.2 → v5.3（互換性問題により変更）
- **主要クレート**:
  - esp-idf-sys v0.36.1
  - esp-idf-hal v0.45.2
  - esp-idf-svc v0.51.0
  - embuild v0.33.1

### 遭遇した問題

#### 1. 浮動小数点 ABI の不一致
**エラー**:
```
can't link single-float modules with soft-float modules
```

**原因**:
- ESP-IDF (C側): libgcc.a が `ilp32f` (hardware float) でコンパイル済み
- Rust側: `build-std` で標準ライブラリをコンパイルする際、soft-float ABI になる
- cc-rs のデフォルトフラグ (`-march=rv32imc -mabi=ilp32`) が ESP-IDF の正しいフラグを上書き

**解決策（ESP-IDF v5.3 では成功）**:
```toml
# .cargo/config.toml
[env]
CRATE_CC_NO_DEFAULTS = "1"  # cc-rs のデフォルトフラグを無効化
ESP_IDF_VERSION = { value = "v5.3" }
```

#### 2. ESP-IDF v5.5.2 との互換性問題
**esp-idf-hal v0.45.2 では以下のエラーが発生**:
- `Core1` が見つからない（ESP32-P4 は dual-core だが認識されない）
- `hys_ctrl_mode` フィールドがない（ESP-IDF v5.5 で GPIO 構造体に追加）
- `_frxt_setup_switch` が見つからない（Xtensa 専用関数、RISC-V には存在しない）

**結論**: esp-idf-hal v0.45.2 は ESP32-P4 + ESP-IDF v5.5.2 に未対応

### 成功した構成（参考：ESP-IDF v5.3）

#### Cargo.toml
```toml
[dependencies]
esp-idf-svc = { version = "0.51", default-features = false }
esp-idf-hal = { version = "0.45" }
esp-idf-sys = { version = "0.36" }

[build-dependencies]
embuild = { version = "0.33", features = ["espidf"] }
```

#### .cargo/config.toml
```toml
[build]
target = "riscv32imafc-esp-espidf"

[target.riscv32imafc-esp-espidf]
linker = "ldproxy"
runner = "espflash flash --monitor"
rustflags = [
    "--cfg", "espidf_time64",
    "-C", "default-linker-libraries",
]

[unstable]
build-std = ["std", "panic_abort"]
build-std-features = ["panic_immediate_abort"]

[env]
ESP_IDF_VERSION = { value = "v5.3" }
MCU = "esp32p4"
CRATE_CC_NO_DEFAULTS = "1"  # 🔑 重要
CFLAGS_riscv32imafc_esp_espidf = "-march=rv32imafc_zicsr_zifencei -mabi=ilp32f"
CXXFLAGS_riscv32imafc_esp_espidf = "-march=rv32imafc_zicsr_zifencei -mabi=ilp32f"
```

### 判断理由

1. **ESP32-P4 の Rust サポートがまだ実験的段階**
   - `riscv32imafc-esp-espidf` ターゲットは Rust で Tier 3（最低保証レベル）
   - Tier 2 への昇格が検討中（2025年初頭時点）

2. **ディスプレイ・カメラのサポートが必要**
   - LVGL、MIPI-DSI、MIPI-CSI などは ESP-IDF (C) で完全サポート
   - Rust の esp-hal (bare-metal) では周辺機器サポートが不十分

3. **開発速度の優先**
   - ESP-IDF (C/C++) であれば確実に動作する
   - Rust エコシステムの成熟を待つ余裕がない

## 参考資料

### 成功した情報源
- [esp-rs/esp-idf-sys Issue #176: RISC-V Compile Errors](https://github.com/esp-rs/esp-idf-sys/issues/176)
  - `CRATE_CC_NO_DEFAULTS=1` の回避策
- [esp-idf-hal CHANGELOG](https://github.com/esp-rs/esp-idf-hal/blob/master/CHANGELOG.md)
  - ESP-IDF v5.5 対応状況
- [Rust on ESP32-P4 Tier 2 Promotion](https://github.com/rust-lang/compiler-team/issues/864)
  - ターゲットの公式サポート状況

### ESP32-P4 公式情報
- [ESP32-P4 Function EV Board](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html)
- [ESP32-P4 ESP-IDF v5.5.2 Resources](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/resources.html)

## 今後の展望

Rust での ESP32-P4 開発は、以下の条件が揃ったら再挑戦する価値があります：

1. **esp-idf-hal が ESP-IDF v5.5+ を完全サポート**（v0.46+ リリース待ち）
2. **`riscv32imafc-esp-espidf` が Tier 2 に昇格**
3. **ESP32-P4 専用の周辺機器ドライバが充実**（MIPI、カメラ等）

---

**最終更新**: 2026年1月12日
**決定**: C/C++ + ESP-IDF での開発に切り替え

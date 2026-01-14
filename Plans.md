# Plans

## プロジェクト: Handy Keyboard Firmware

- **目的**: ESP32-P4ベースの多機能ハンディキーボード/コントローラ
- **言語**: C/C++ + ESP-IDF v5.5.2
- **GUI**: LVGL v9.2.2

---

## 完了したフェーズ

| Phase | 内容 | 完了日 |
|-------|------|--------|
| 0 | Rust試行→C/C++方針転換 | 2026-01-12 |
| 1 | プロジェクト構造セットアップ | 2026-01-12 |
| 2 | I2Cコンポーネント整備（RTC, Touch, Audio） | 2026-01-12 |
| 3-1 | BSP統合・LVGL基盤 | 2026-01-12 |
| 3-2 | SquareLine Studio UI統合 | 2026-01-13 |
| 3-3 | 画面ナビゲーション実装 | 2026-01-13 |
| 3-4 | UIカスタマイズ実装 | 2026-01-13 |

**詳細記録**: `03.firmware/04.integratedAPP/README.md`

---

## ✅ 完了: フェーズ4-5 BLE HIDキーボード実装

### 実装内容（2026-01-14）

- ✅ ble_hidコンポーネント作成（ESP-Hosted + NimBLE統合）
- ✅ 日本語フリック入力→ローマ字変換
- ✅ AtoZキーボード対応
- ✅ カーソルキー・ショートカット対応
- ✅ UIイベントハンドラ統合

### フリック方向マッピング
| 操作 | 文字 |
|------|------|
| 中央(tap) | あ |
| 左 | い |
| 上 | う |
| 右 | え |
| 下 | お |

---

## 🟢 cc:WIP 次のタスク

### 実装済みナビゲーション

| 画面 | 要素 | 遷移先 |
|------|------|--------|
| Menu | KeyBoards | JPKeyboard |
| Menu | Clock | AnalogClock |
| Menu | Setthing | Settings |
| JPKeyboard | Image2(絵) | Menu |
| JPKeyboard | Panel17(あ/a) | AtoZ |
| JPKeyboard | Panel12(<+>) | Cursor |
| AtoZ | HeaderPanel1 | JPKeyboard |
| AtoZ | ボタンID35 | JPKeyboard |
| Cursor | Image6(絵) | Menu |
| Cursor | Panel31([a]) | AtoZ |
| Cursor | Panel36(あ) | JPKeyboard |
| Clock | Image11 | Menu |
| DateAndTime | Back | Menu |
| Settings | 戻るボタン | Menu |

---

## 🟢 cc:TODO フェーズ4〜11: 今後の作業

### Phase 4: 入力処理
- タッチパッド入力（タップ、ジェスチャー、フリック）
- キーマップ（日本語フリック、QWERTY）

### Phase 5: HID出力
- USB HID（Boot/NKRO/Mouse/Consumer）
- BLE HID（5接続先プロファイル）

### Phase 6: 時計モード
- RTC時刻同期（NTP→DS3231M）
- デジタル/アナログ時計表示
- アラーム機能

### Phase 7: オーディオ
- ES8311 + I2S
- MP3/AAC再生

### Phase 8: WiFi
- SSID設定・接続
- Wake-on-LAN

### Phase 9: SDカード
- MJPEG再生
- USBマスストレージ

### Phase 10: 設定UI
- NVS設定値管理
- 各種設定画面

### Phase 11: 統合テスト
- 全機能動作確認
- パフォーマンス最適化

---

## 完了タスク履歴

- ✅ ハーネス初期セットアップ
- ✅ Rust試行・検証→C/C++方針決定
- ✅ I2C周辺デバイス通信確認（RTC, Touch, Audio）
- ✅ LVGL + BSP統合
- ✅ SquareLine Studio UI統合
- ✅ 画面ナビゲーション・UIカスタマイズ

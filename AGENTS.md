# Agents Workflow

## プロジェクト概要
ESP32-P4 + ESP32-C6 を使用した多機能ハンディキーボード/コントローラのファームウェア開発プロジェクトです。

## 運用モード
**Solo モード**: Claude Code のみで開発を完結します。

## 開発フロー

### 1. 計画フェーズ
```
/plan-with-agent または手動で Plans.md に追加
↓
タスクを cc:TODO マーカーでマーク
```

### 2. 実装フェーズ
```
/work でタスク実行
↓
cc:TODO → cc:WIP → 完了（マーカー削除）
```

### 3. レビューフェーズ
```
/harness-review で品質チェック
↓
指摘があれば修正
```

### 4. ビルド・検証
```
/verify でビルド検証
↓
ESP-IDF スキルが自動適用
```

## マーカー凡例
| マーカー | 状態 | 説明 |
|---------|------|------|
| `cc:TODO` | 未着手 | Claude Code が実行予定 |
| `cc:WIP` | 作業中 | 実装中 |
| `cc:blocked` | ブロック中 | 依存タスク待ち |

## よく使うコマンド
- `/plan-with-agent <内容>`: 新機能の計画を作成
- `/work`: Plans.md のタスクを実行
- `/harness-review`: コードレビュー
- `/verify`: ビルド検証
- `/sync-status`: 進捗確認・Plans.md 更新

## ディレクトリ構成
```
01.docs/          # ドキュメント
03.firmware/      # ファームウェアソースコード
  04.integratedAPP/ # メイン統合アプリ
04.reference/     # 参考資料
.claude/          # Claude Code 設定・スキル
```

## 技術スタック
- **Framework**: ESP-IDF v5.5.2
- **Language**: Rust (esp-idf-hal + LVGL Rust binding)
- **Platform**: ESP32-P4 (メイン) + ESP32-C6 (無線)
- **Build**: Ubuntu + bash + cargo/idf.py
- **Communication**: ESP-Hosted over SDIO

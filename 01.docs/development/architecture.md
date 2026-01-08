# アーキテクチャ設計

## システム概要

```
┌─────────────────────────────────────────┐
│         ESP32-P4 (Main MCU)             │
│  ┌────────────────────────────────────┐ │
│  │   Application Layer                │ │
│  │  - Keyboard Controller             │ │
│  │  - Mouse Controller                │ │
│  │  - Clock/Timer                     │ │
│  │  - Settings Manager                │ │
│  │  - WoL (Wake-on-LAN)               │ │
│  └────────────────────────────────────┘ │
│  ┌────────────────────────────────────┐ │
│  │   Service Layer                    │ │
│  │  - Network Service (WiFi/BLE)      │ │
│  │  - Audio Service                   │ │
│  │  - Display Service                 │ │
│  │  - Input Service                   │ │
│  │  - Storage Service (SD)            │ │
│  └────────────────────────────────────┘ │
│  ┌────────────────────────────────────┐ │
│  │   Hardware Abstraction Layer (HAL) │ │
│  │  - hal_i2c                         │ │
│  │  - hal_audio (I2S + ES8311)        │ │
│  │  - hal_display (MIPI-DSI)          │ │
│  │  - hal_touch (GT911)               │ │
│  │  - hal_sd (SDMMC)                  │ │
│  │  - hal_net (ESP-Hosted wrapper)    │ │
│  └────────────────────────────────────┘ │
└─────────────────────────────────────────┘
                    │
                    │ SDIO
                    ↓
┌─────────────────────────────────────────┐
│      ESP32-C6 (Radio MCU)               │
│  ESP-Hosted MCU Slave                   │
│  - WiFi 6                               │
│  - BLE 5.4                              │
└─────────────────────────────────────────┘
```

## 通信プロトコル

### ESP32-P4 ↔ ESP32-C6: ESP-Hosted over SDIO
- **プロトコル**: ESP-Hosted MCU
- **Transport**: SDIO (4-bit mode)
- **機能**:
  - WiFi: STA/AP mode
  - BLE: NimBLE via VHCI
  - 双方向通信

## レイヤー設計

### 1. Application Layer
- **責務**: ユーザー機能の実装
- **特徴**: ビジネスロジックに集中
- **依存**: Service Layerのみに依存

### 2. Service Layer
- **責務**: 機能の抽象化・複数HALの統合
- **特徴**: 再利用可能なサービス
- **依存**: HAL Layerのみに依存

### 3. Hardware Abstraction Layer (HAL)
- **責務**: ハードウェアの直接制御
- **特徴**: ハードウェア依存コードの隔離
- **依存**: ESP-IDF APIのみに依存

## ファイル分割ポリシー

### 統合アプリ (`04.integratedAPP`)
- **.cファイルは300行程度**を目安
- 1ファイル = 1つの責務
- 依存関係を最小化

### ディレクトリ構成例
```
03.firmware/04.integratedAPP/
├── main/
│   ├── main.c                    # エントリーポイント
│   ├── app/                      # Application Layer
│   │   ├── app_keyboard.c/h
│   │   ├── app_mouse.c/h
│   │   ├── app_clock.c/h
│   │   └── app_settings.c/h
│   ├── service/                  # Service Layer
│   │   ├── srv_network.c/h
│   │   ├── srv_audio.c/h
│   │   ├── srv_display.c/h
│   │   ├── srv_input.c/h
│   │   └── srv_storage.c/h
│   └── hal/                      # Hardware Abstraction Layer
│       ├── hal_i2c.c/h
│       ├── hal_audio.c/h
│       ├── hal_display.c/h
│       ├── hal_touch.c/h
│       ├── hal_sd.c/h
│       └── hal_net.c/h
└── CMakeLists.txt
```

## 設定管理

### NVS (Non-Volatile Storage)
- WiFi設定
- BLE接続先（最大5個）
- 時計設定（アラーム等）
- UI設定

### MicroSD Card
- 画像ファイル (JPEG)
- 動画ファイル (MJPEG)
- 音楽ファイル (MP3)
- ユーザーデータ

## パフォーマンス考慮事項

### メモリ
- **PSRAM**: 16MB (200MHz)
  - フレームバッファ
  - 画像/動画デコード
  - 音楽バッファ
- **Internal RAM**: クリティカルなタスク

### タスク優先度（予定）
1. Audio Task (最高)
2. Display Task
3. Input Task
4. Network Task
5. Storage Task (最低)

## 今後の拡張性
- カメラ機能の追加
- 音声認識機能
- 追加センサーの統合

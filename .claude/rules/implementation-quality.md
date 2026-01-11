# 実装品質保護ルール

## 目的
形骸化した実装を防ぎ、実用的なコードを維持します。

## 禁止事項

### 1. ダミー実装の放置
❌ **禁止**: 常に成功する・何もしない実装
```rust
// NG例
fn init_device() -> Result<(), EspError> {
    Ok(())  // 実際には何も初期化していない
}

fn process_data(data: &[u8]) {
    // 何もしない
}
```

✅ **許可**: 実際に動作する実装
```rust
// OK例
fn init_device(i2c: &I2cDriver) -> Result<(), EspError> {
    i2c_master_init()?;
    codec_configure(i2c)?;
    Ok(())
}

fn process_data(data: &[u8], buffer: &mut Vec<u8>) {
    buffer.clear();
    buffer.extend_from_slice(data);
}
```

### 2. 過剰な抽象化
❌ **禁止**: 使われない柔軟性のための複雑化
```rust
// NG例: 1箇所でしか使わないのに汎用的すぎる
trait GenericHandler {
    fn init(&mut self) -> Result<(), EspError>;
    fn process(&mut self, data: Box<dyn Any>);
    fn cleanup(&mut self);
}

struct HandlerFactory;  // 複雑すぎる
impl HandlerFactory {
    fn create(handler_type: HandlerType) -> Box<dyn GenericHandler> { ... }
}
```

✅ **許可**: シンプルで直感的な実装
```rust
// OK例: 必要最小限
struct Keyboard { ... }
impl Keyboard {
    fn new() -> Result<Self, EspError> { ... }
    fn process(&mut self) { ... }
}
```

### 3. 不必要な依存
❌ **禁止**: 使わない機能を含むクレートの追加
```rust
// NG例: 簡単な計算のために重いクレートを導入
use complex_math_library::*;

fn calculate_average(a: i32, b: i32) -> i32 {
    complex_math_library::mean(a, b)  // オーバーキル
}
```

✅ **許可**: 標準機能で実装
```rust
// OK例
fn calculate_average(a: i32, b: i32) -> i32 {
    (a + b) / 2
}
```

### 4. エラーハンドリングの欠如
❌ **禁止**: エラーを無視する
```rust
// NG例
fn init_system() {
    let _ = gpio_config(&io_conf);  // 結果を無視
    i2c_master_init();  // unwrap() や expect() なしで放置
}
```

✅ **許可**: 適切なエラーハンドリング
```rust
// OK例
use log::error;

fn init_system() -> Result<(), EspError> {
    gpio_config(&io_conf)
        .map_err(|e| {
            error!("GPIO config failed: {:?}", e);
            e
        })?;

    i2c_master_init()
        .map_err(|e| {
            error!("I2C init failed: {:?}", e);
            e
        })?;

    Ok(())
}
```

## 推奨事項

### シンプルさを優先
- 1つの関数は1つの責務
- ネストは3段階まで
- 関数は50行以内を目安に

### 意図を明確に
- 変数名・関数名は目的がわかるように
- マジックナンバーは定数化
- 複雑なロジックにはコメント

### Rust + ESP-IDF のベストプラクティスに従う
- エラーは `Result<T, EspError>` で返す
- ログは `log` クレート（`info!`, `error!` など）を使う
- タスクは適切な優先度・スタックサイズで作成
- `unsafe` は最小限に、使用時は理由をコメント

## レビュー時の確認事項
- [ ] ダミー実装になっていないか
- [ ] 不必要に複雑になっていないか
- [ ] エラーハンドリングが適切か（Result 型、? 演算子の活用）
- [ ] `cargo fmt` / `cargo clippy` が通るか
- [ ] `unsafe` の使用が最小限で、理由が明記されているか
- [ ] ESP-IDF のガイドラインに沿っているか

---

**原則**: 動くコードをシンプルに書く。複雑さは必要になってから追加する。

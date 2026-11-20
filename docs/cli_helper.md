# CLIHelper

`CLIHelper`は、Arduinoの`Stream`から改行区切りのコマンドを読み取り、登録した関数へ引数を渡す軽量なCLIヘルパーです。既定では`Serial`を使用します。

## 基本的な使い方

```cpp
#include <Arduino.h>
#include "cli_helper.hpp"

CLIHelper cli("> ", true, Serial, 128);
String deviceName = "ESP32";

void setup() {
  Serial.begin(115200);

  cli.registerHandler(
      "status",
      [](const String& args) {
        (void)args;
        Serial.printf("Device name: %s\n", deviceName.c_str());
      },
      "Show device status");

  cli.registerHandler(
      "name",
      [](const String& args) {
        deviceName = args;
        Serial.printf("Device name set to: %s\n", deviceName.c_str());
      },
      "Set device name: name <value>");

  cli.registerHandler(
      "help",
      [](const String& args) {
        (void)args;
        Serial.print(cli.getHelpString());
      },
      "List available commands");

  cli.begin();
}

void loop() {
  cli.feedInput();
}
```

シリアルモニターを115200 baud、改行送信に設定し、例えば次のように入力します。

```text
status
name Living Room
help
```

コマンド名と引数は最初の空白で分割されます。したがって、`name Living Room`のハンドラーには`Living Room`が渡されます。引数内の空白は保持されます。

## API

### `CLIHelper(prompt, local_echo, stream, max_buf)`

| 引数 | 既定値 | 説明 |
| --- | --- | --- |
| `prompt` | `"> "` | `begin()`時と各入力行の処理後に表示する文字列。空文字列なら非表示。 |
| `local_echo` | `false` | `true`にすると入力文字をStreamへ書き戻す。 |
| `stream` | `Serial` | 入力と出力に使用するArduino `Stream`。 |
| `max_buf` | `512` | 1行の最大入力バイト数。超過したバイトは無視される。UTF-8では文字数ではなくバイト数。 |

### `begin()`

内部入力バッファを予約し、初期プロンプトを表示します。Streamの準備後、通常は`setup()`から一度呼びます。

### `registerHandler(command, handler, short_help)`

コマンドを登録します。ハンドラーの型は`void(const String&)`で、コマンド名を除いた引数文字列を受け取ります。同じ名前を再登録した場合は既存の登録を置き換えます。`short_help`を省略すると`Call <command>`が使われます。

### `getHelpString()`

登録済みコマンドを`command: short_help`形式の改行区切り文字列として返します。`help`コマンドは自動登録されないため、必要な場合は例のように登録します。

### `feedInput()`

Streamに届いている入力を読み取ります。改行で1行を確定し、該当するハンドラーを同期的に実行します。`loop()`から繰り返し呼び出してください。空行は実行せず、プロンプトだけを表示します。

### `setStream(stream)`

以降の入力と出力に使うStreamを切り替えます。CLIHelperはStreamへの参照を保持するため、渡すStreamオブジェクトはCLIHelperを使う間有効でなければなりません。切り替え時点ではプロンプトを表示しません。

### `unhandledCommand(command, args)`

未登録コマンドの既定処理です。登録済みコマンド一覧と、不明なコマンド名および引数を現在のStreamへ出力します。

## 入力処理の仕様と注意点

- コマンド名は大文字・小文字を区別します。
- 区切り文字は最初の半角スペース1個です。引数の先頭に続く空白があっても削除されません。
- `\r`は無視され、`\n`でコマンドを実行します。`\b`とDEL（`0x7f`）は直前の入力文字を削除します。
- `local_echo`が`false`の場合、CLIHelperは入力文字を表示しません。端末側の表示設定に応じて有効にしてください。
- コマンドハンドラーは`feedInput()`を呼んだタスク上で同期実行されます。長い処理を行う場合は、その間ほかの`loop()`処理が止まる点に注意してください。
- `max_buf`を超えた入力はエラーを出さず無視されます。必要な最大コマンド長に合わせて設定してください。

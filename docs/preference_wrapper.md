# preference_wrapper

`preference_wrapper.hpp`は、ESP32 Arduinoの`Preferences`（NVS）に保存する値を`NvsProperty<T>`として扱うための小さなラッパーです。`DEFINE_NVS_PROP`マクロでクラスのメンバーとして宣言し、`init()`で名前空間・キー・既定値を設定します。

## 基本的な使い方

```cpp
#include <Arduino.h>
#include "preference_wrapper.hpp"

class AppSettings {
public:
  void begin() {
    device_name.init("my_app", "device_name", String("ESP32"));
    led_pin.init("my_app", "led_pin", uint8_t(8));
    sample_period.init("my_app", "sample_period", uint32_t(1000));
  }

  DEFINE_NVS_PROP(String, device_name);
  DEFINE_NVS_PROP(uint8_t, led_pin);
  DEFINE_NVS_PROP(uint32_t, sample_period);
};

AppSettings settings;

void setup() {
  Serial.begin(115200);
  settings.begin();

  Serial.printf("Device: %s\n", settings.device_name.c_str());
  Serial.printf("LED pin: %u\n", static_cast<unsigned>(settings.led_pin));
  Serial.printf("Sample period: %lu ms\n",
                static_cast<unsigned long>(settings.sample_period));

  settings.device_name = String("Room Sensor");
  settings.led_pin = uint8_t(3);
  settings.sample_period = uint32_t(500);
}

void loop() {}
```

`init()`は各プロパティの使用前に呼び出してください。再起動後はNVSに保存された値が読み込まれます。

## API

### `DEFINE_NVS_PROP(Type, Name)`

`NvsProperty<Type> Name`を宣言するマクロです。メンバー宣言を簡潔にするもので、初期化は行いません。

### `init(namespace_name, key_name, default_value)`

NVSの名前空間とキーを設定し、保存済みの値を読み込みます。キーが存在しない場合は`default_value`をプロパティの値にします。

**既定値はNVSへ書き込まれません。** 初期値を使ったあとに再起動しても、設定を代入して保存していなければ、キーは引き続き未登録です。

`namespace_name`と`key_name`はポインターとして保持されるため、プロパティを使っている間有効な文字列（通常は文字列リテラル）を渡してください。

### 値の読み書き

- 読み取りはプロパティから型への変換で行います。例えば`static_cast<uint8_t>(settings.led_pin)`のように使えます。
- `NvsProperty<String>`は`c_str()`も提供します。`Serial.printf`などで使えます。
- 代入すると、プロパティの値を更新してNVSへ保存します。例えば`settings.led_pin = uint8_t(3);`です。

## 保存形式と注意点

- `String`は`Preferences::getString` / `putString`で保存されます。
- その他の型は`Preferences::getBytes` / `putBytes`による生バイト保存です。`uint8_t`、`uint32_t`、`float`、`bool`など、固定サイズで単純な値に限定してください。ポインター、`String`以外の動的オブジェクト、可変長データには使わないでください。
- 生バイト保存には型情報やバージョン情報がありません。既存キーの型やサイズを変更する場合は、別のキーを使うか、保存データの移行・消去を行ってください。
- `Preferences`の読み書き結果はこのラッパー内で確認していません。NVSのエラーを呼び出し側へ返すAPIもありません。
- NVSの名前空間・キー名には、ESP32 Preferences側の長さ制限があります。異なる設定には適切に異なるキー名を割り当ててください。

## アプリの設定クラスで使う例

```cpp
class AppPreference {
public:
  void init() {
    ble_name.init("esp32_ir_ble", "ble_name", String("ESP32-IR-BLE"));
    ir_pin.init("esp32_ir_ble", "ir_pin", uint8_t(4));
  }

  DEFINE_NVS_PROP(String, ble_name);
  DEFINE_NVS_PROP(uint8_t, ir_pin);
};

AppPreference apppref;

void setup() {
  apppref.init();
  Serial.println(apppref.ble_name.c_str());
  Serial.println(static_cast<unsigned>(apppref.ir_pin));

  apppref.ir_pin = uint8_t(5); // NVSへ保存
}
```

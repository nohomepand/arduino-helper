# RingBuffer

`RingBuffer<T>`は、生成時に指定した容量を持つFIFOバッファです。格納領域は生成時に確保し、要素オブジェクトは追加時に構築します。コピーはできませんが、ムーブできます。スレッドセーフではありません。

## 使用例

```cpp
#include <memory>
#include "ringbuffer.hpp"

RingBuffer<std::unique_ptr<int>> values(4);

void example() {
  values.emplace_back(new int(42));

  std::unique_ptr<int> value;
  if (values.pop_front(value)) {
    // valueが取り出したintの所有権を持つ
  }

  values.emplace_back(new int(7));
  values.purge(); // 格納中のunique_ptrと、その所有するintを破棄
}
```

`T`にはクラス型やスカラー型を指定できます。必要な型要件は使うAPIによって異なり、`push_back(const T&)`はコピー構築、`push_back(T&&)`はムーブ構築、`pop_front(out)`はムーブ代入を必要とします。`emplace_back()`で構築して`purge()`するだけなら、コピー不能な型も格納できます。生のC配列型は要素として使えないため、固定長配列には`std::array<uint8_t, 10>`などを使ってください。

## API

### `RingBuffer<T>(capacity)`

バッファを作成します。`capacity`は1以上でなければなりません。領域は一度確保され、要素の出し入れでは再確保しません。

### `push_back(value)`

値をコピーまたはムーブして末尾に追加します。満杯なら`false`を返し、バッファを変更しません。

### `emplace_back(args...)`

引数を転送して末尾に要素を直接構築します。満杯なら`false`を返します。要素の構築に失敗した場合、要素数とインデックスは更新されません。

### `pop_front(out)`

先頭要素を`out`へムーブ代入し、バッファ内の要素を破棄します。空なら`false`、取り出せた場合は`true`を返します。

### `size()` / `capacity()` / `operator bool()`

`size()`は現在の要素数、`capacity()`は最大要素数を返します。bool変換はバッファが空でない場合に`true`です。

### `purge()`

格納中の要素をすべて破棄して空にします。各要素のデストラクターが呼ばれ、要素が所有するリソースも解放されます。バッファ領域自体は保持し、再利用します。領域は`RingBuffer`の破棄時に解放されます。

### ムーブ

コピーコンストラクターとコピー代入は無効です。ムーブするとバッファ領域と格納中の要素の所有権が移り、移動元は容量・要素数ともに0になります。

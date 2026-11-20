# thread_helper

`thread_helper.hpp`は、FreeRTOSのmutexとtaskをC++クラスから使うための補助クラスを`rtos`名前空間に提供します。

- `rtos::Mutex` / `rtos::LockGuard`: タスク間で共有するデータの排他制御
- `rtos::MutexEchange<T>`: 値を1つだけ保持する、上書き可能なメールボックス
- `rtos::Task`: 派生クラスの`run()`をFreeRTOSタスクとして実行

## Taskの使用例

```cpp
#include <Arduino.h>
#include "thread_helper.hpp"

rtos::Mutex counterMutex;
uint32_t counter = 0;

class CounterTask : public rtos::Task {
public:
  CounterTask() : rtos::Task("counter", 4096, 1) {}

protected:
  void run() override {
    while (true) {
      {
        rtos::LockGuard lock(counterMutex);
        if (lock.isLocked()) {
          ++counter;
        }
      }
      sleepMs(1000);
    }
  }
};

CounterTask counterTask;

void setup() {
  Serial.begin(115200);
  if (!counterTask.start()) {
    Serial.println("Failed to start counter task");
  }
}

void loop() {
  {
    rtos::LockGuard lock(counterMutex);
    if (lock.isLocked()) {
      Serial.printf("Counter: %lu\n", static_cast<unsigned long>(counter));
    }
  }
  delay(2000);
}
```

`Task`派生クラスは`run()`を実装します。`run()`が戻るとタスクは自己削除されます。`start()`はタスクを作成できた場合に`true`を返し、起動済みの場合や作成失敗時は`false`を返します。

`start(coreId)`で実行コアを指定できます。コア指定はFreeRTOSがマルチコア構成の場合に使われます。ユニコア構成では通常の`xTaskCreate()`が使われ、指定は適用されません。

## ラムダを実行する `rtos::run()`

```cpp
void setup() {
  Serial.begin(115200);

  if (!rtos::run("hello", []() {
        while (true) {
          Serial.println("hello");
          delay(1000);
        }
      })) {
    Serial.println("Failed to start hello task");
  }
}
```

`rtos::run(name, callback)`はdetachedタスクを作成し、作成成否を`bool`で返します。返されたタスクオブジェクトを保持する必要はありません。コールバックが戻ると関数オブジェクトを解放してタスクが終了します。繰り返し実行したい処理は、例のようにコールバック内でループしてください。

この簡易APIにはタスク停止用のハンドルがありません。後から停止する必要がある場合は、`Task`派生クラスを作って`start()` / `stop()`を使ってください。`Task::stop()`は協調停止ではなく即時削除です。

## MutexとLockGuard

```cpp
rtos::Mutex mutex;
int sharedValue = 0;

void updateSharedValue() {
  rtos::LockGuard lock(mutex, 20); // 最大20 ms待つ
  if (!lock.isLocked()) {
    return; // timeoutまたはmutex生成失敗
  }

  ++sharedValue;
} // lockのデストラクターがmutexを解放
```

- `Mutex::lock(timeoutMs)`はミリ秒単位で待ち、取得成否を返します。既定値`portMAX_DELAY`では無期限に待ちます。
- `Mutex::unlock()`はロックを取得したタスクから呼び出してください。通常は`LockGuard`を使うと、スコープ終了時に解放できます。
- 有限のタイムアウトを指定した場合は、`LockGuard::isLocked()`を確認してください。
- mutexはFreeRTOSの通常タスクから使うもので、ISRからは使用できません。

## MutexEchangeの使用例

```cpp
rtos::MutexEchange<uint32_t> pendingCommand;

// あるタスクから値を通知する
pendingCommand.set(42);

// 別のタスクから保留値を一度だけ取り出す
uint32_t command;
if (pendingCommand.take(command)) {
  Serial.printf("Command: %lu\n", static_cast<unsigned long>(command));
}
```

- `set(value)`は保留中の値を上書きします。複数値をキューイングする機能はありません。
- `has()`は保留の有無を確認しますが、値を消費しません。
- `take(result)`は値があれば取り出して保留状態を解除します。
- ブロックして通知を待つ機能はなく、ISRからも呼び出せません。`set()`、`has()`、`take()`は通常タスク間での受け渡しに使います。

## 制約と注意点

- `Task::stop()`は協調停止要求ではなく、FreeRTOSの`vTaskDelete()`でタスクを即時削除します。`run()`内の後処理やC++ローカル変数のデストラクターが実行される保証はありません。
- `Task`オブジェクトはタスクの実行中、有効なままにしてください。派生クラスのメンバーが破棄される前に、必要なら派生クラスのデストラクターから`stop()`を呼び出します。
- `run()`内で終了条件を使う場合は、共有フラグなどをmutex等で同期し、`run()`自身が戻るように設計できます。このクラスにはjoin/wait APIはありません。
- `stackSize`はESP32 Arduino/ESP-IDFではバイト単位です。タスクの処理に必要なスタック量を確保してください。
- `Mutex`の作成失敗は`lock()`の`false`で確認できます。`MutexEchange`の各メソッドはロック失敗を呼び出し元に通知しないため、メモリ不足などでmutex作成に失敗した場合の利用には注意が必要です。

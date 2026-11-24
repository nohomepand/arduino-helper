#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <stdint.h>
#include <functional>
#include <utility>

namespace rtos {

/** FreeRTOS mutexのRAIIラッパー。 */
class Mutex {
public:
    Mutex() {
        handle_ = xSemaphoreCreateMutex();
    }

    ~Mutex() {
        if (handle_ != nullptr) {
            vSemaphoreDelete(handle_);
        }
    }

    // コピー禁止（二重解放防止）
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    /** mutexを取得する。timeoutMsはミリ秒。既定では無期限に待つ。 */
    bool lock(uint32_t timeoutMs = portMAX_DELAY) {
        if (handle_ == nullptr) return false;
        TickType_t ticks = (timeoutMs == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeoutMs);
        return xSemaphoreTake(handle_, ticks) == pdTRUE;
    }

    /** 取得済みmutexを解放する。取得したタスクから呼び出すこと。 */
    void unlock() {
        if (handle_ != nullptr) {
            xSemaphoreGive(handle_);
        }
    }

private:
    SemaphoreHandle_t handle_ = nullptr;
};

/** スコープ終了時に、取得に成功したmutexを自動解放する。 */
class LockGuard {
public:
    explicit LockGuard(Mutex& mutex, uint32_t timeoutMs = portMAX_DELAY)
        : mutex_(mutex) {
        locked_ = mutex_.lock(timeoutMs);
    }

    ~LockGuard() {
        if (locked_) {
            mutex_.unlock();
        }
    }

    /** mutexを取得できた場合にtrueを返す。 */
    bool isLocked() const { return locked_; }

    // コピー禁止
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    Mutex& mutex_;
    bool locked_ = false;
};

/**
 * @brief 値とmutexをまとめ、スレッドセーフにアクセスするためのコンテナ。
 *
 * lock()で取得したGuardedが生存している間、保持する値へのアクセスを排他します。
 * GuardedのisLocked()を確認してから値にアクセスしてください。
 *
 * @tparam T 保持する値の型。
 */
template<typename T>
class SharedObject {
public:
    /**
     * @brief 値へのアクセス権とmutexロックを保持するRAIIガード。
     *
     * コピーはできません。移動構築するとロックの解放責任も移動します。
     */
    class Guarded {
    public:
        /**
         * @brief mutexを取得し、指定された値を保護するガードを作る。
         * @param mutex 保護に使うmutex。
         * @param value 保護対象の値。
         * @param timeoutMs mutex取得を待つ時間（ミリ秒）。portMAX_DELAYは無期限。
         */
        Guarded(Mutex& mutex, T& value, uint32_t timeoutMs)
            : mutex_(&mutex), value_(&value), locked_(mutex.lock(timeoutMs)) {}

        /** @brief ロックを取得済みの場合、mutexを解放する。 */
        ~Guarded() {
            unlock();
        }

        /** @brief ガードのコピー構築を禁止する。 */
        Guarded(const Guarded&) = delete;

        /** @brief ガードのコピー代入を禁止する。 */
        Guarded& operator=(const Guarded&) = delete;

        // Move assignment is explicitly deleted to prevent accidental resource transfer after construction.
        // Only move construction is allowed for Guarded.
        /** @brief ガードのムーブ代入を禁止する。 */
        Guarded& operator=(Guarded&&) = delete;

        /**
         * @brief ロックの解放責任を移動する。
         * @param other 移動元。移動後はロックを解放しない。
         */
        Guarded(Guarded&& other) noexcept
            : mutex_(other.mutex_), value_(other.value_), locked_(other.locked_) {
            other.mutex_ = nullptr;
            other.value_ = nullptr;
            other.locked_ = false;
        }

        /**
         * @brief mutexを取得できたかを返す。
         * @return 取得できた場合はtrue。
         */
        bool isLocked() const { return locked_; }

        /**
         * @brief 保護対象のメンバーにポインター形式でアクセスする。
         * @return 保護対象へのポインター。アクセス前にisLocked()を確認すること。
         */
        T* operator->() const { return value_; }

        /**
         * @brief 保護対象に参照形式でアクセスする。
         * @return 保護対象への参照。アクセス前にisLocked()を確認すること。
         */
        T& operator*() const { return *value_; }

        /**
         * @brief 保護対象への参照に暗黙変換する。
         * @return 保護対象への参照。アクセス前にisLocked()を確認すること。
         */
        operator T&() const { return *value_; }

        /**
         * @brief 保護対象への参照を取得する。
         * @return 保護対象への参照。アクセス前にisLocked()を確認すること。
         */
        T& get() const { return *value_; }

    private:
        /** 取得済みの場合だけmutexを解放する。 */
        void unlock() {
            if (locked_ && mutex_ != nullptr) {
                mutex_->unlock();
                locked_ = false;
            }
        }

        /** ロックの取得・解放に使うmutex。 */
        Mutex* mutex_;

        /** ロックで保護される値。 */
        T* value_;

        /** このガードがロックを所有しているか。 */
        bool locked_;
    };

    /** @brief Tをデフォルト初期化したSharedObjectを作る。 */
    SharedObject() : mutex_(), value_() {}

    /**
     * @brief 指定した値のコピーを保持するSharedObjectを作る。
     * @param value 初期値。
     */
    explicit SharedObject(const T& value) : mutex_(), value_(value) {}

    /**
     * @brief 指定した値をムーブして保持するSharedObjectを作る。
     * @param value 初期値。ムーブ後の状態は未規定。
     */
    explicit SharedObject(T&& value) : mutex_(), value_(std::move(value)) {}

    /**
     * @brief 値への排他アクセス用ガードを取得する。
     * @param timeoutMs mutex取得を待つ時間（ミリ秒）。既定値portMAX_DELAYは無期限。
     * @return mutexのロックを保持するGuarded。取得失敗時はisLocked()がfalse。
     */
    [[nodiscard]]
    Guarded lock(uint32_t timeoutMs = portMAX_DELAY) {
        return Guarded(mutex_, value_, timeoutMs);
    }

private:
    Mutex mutex_;
    T value_;
};

/**
 * @brief タスク間で値を1つ受け渡す、上書き可能なメールボックス。
 *
 * set()は保留中の値を上書きし、take()は値を読み取って保留状態を解除する。
 * 待ち合わせやISRからの呼び出しには対応しない。
 */
template<typename T>
class MutexEchange {
public:
    MutexEchange() : mutex_(), set_(false) {}

    /** 値を設定する。未取得の値があれば上書きする。 */
    bool set(T value) {
        LockGuard guard(mutex_);
        if (guard.isLocked()) {
            value_ = value;
            set_ = true;
            return true;
        } else
            return false;
    }

    /** 値が保留中か確認する。値は消費しない。 */
    bool has() {
        LockGuard guard(mutex_);
        return set_;
    }

    /** 保留中の値をresultに取り出し、保留状態を解除する。 */
    bool take(T& result) {
        LockGuard guard(mutex_);
        if (set_) {
            result = value_;
            set_ = false;
            return true;
        } else
            return false;
    }
private:
    Mutex mutex_;
    bool set_;
    T value_;
};

/**
 * @brief run()をFreeRTOSタスクとして実行するための基底クラス。
 *
 * 派生クラスはrun()を実装し、Taskオブジェクトをタスクの実行中ずっと
 * 有効に保つこと。
 */
class Task {
public:
    /**
     * @param name FreeRTOSタスク名。Taskの使用中有効な文字列にする。
     * @param stackSize FreeRTOSタスクスタックサイズ。ESP32 Arduinoではバイト単位。
     * @param priority FreeRTOSタスク優先度。
     */
    Task(const char* name, uint32_t stackSize = 4096, UBaseType_t priority = 1)
        : name_(name), stackSize_(stackSize), priority_(priority) {}

    virtual ~Task() {
        stop();
    }

    /**
     * タスクを開始する。既に開始済み、または作成に失敗した場合はfalse。
     * マルチコア構成ではcoreIdに割り当てるコアを指定できる。
     */
    bool start(BaseType_t coreId = tskNO_AFFINITY) {
        if (handle_ != nullptr) return false; // 既に起動済み

        BaseType_t res;
#if CONFIG_FREERTOS_UNICORE
        res = xTaskCreate(taskEntry, name_, stackSize_, this, priority_, &handle_);
#else
        // ESP32などのマルチコア環境でコア指定する場合
        res = xTaskCreatePinnedToCore(taskEntry, name_, stackSize_, this, priority_, &handle_, coreId);
#endif
        return res == pdPASS;
    }

    /**
     * タスクを削除する。協調停止ではないため、run()は途中で即時終了される。
     * 既に終了している場合は何もしない。
     */
    void stop() {
        if (handle_ != nullptr) {
            TaskHandle_t temp = handle_;
            handle_ = nullptr;
            vTaskDelete(temp);
        }
    }

protected:
    /** 派生クラスが実装するタスク本体。戻るとタスクは自己削除する。 */
    virtual void run() = 0;

    /** タスクを指定ミリ秒間ブロックする。 */
    void sleepMs(uint32_t ms) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }

private:
    // FreeRTOSに渡す静的エントリーポイント
    static void taskEntry(void* pvParameters) {
        Task* task = static_cast<Task*>(pvParameters);
        if (task != nullptr) {
            task->run();
        }
        // run() を抜けた場合は安全にタスクを自己削除
        task->handle_ = nullptr;
        vTaskDelete(nullptr);
    }

    const char* name_;
    uint32_t stackSize_;
    UBaseType_t priority_;
    TaskHandle_t handle_ = nullptr;
};

/**
 * run() 関数で使われる Taskの簡単なラッパー
 */
class SimpleTask : public Task {
public:
    SimpleTask(const char* name, std::function<void()> runner) : rtos::Task(name, 4096, 1), _runner(std::move(runner)) {}

protected:
    void run() override {
        _runner();
    }

private:
    std::function<void()> _runner;
};

/**
 * 関数 runnerを本体として新たなスレッドを開始する。
 * Taskオブジェクトをタスクの実行中ずっと有効に保つこと。
 */
SimpleTask run(const char* name, std::function<void()> runner) {
    SimpleTask task(name, runner);
    task.start();
    return task;
}

}

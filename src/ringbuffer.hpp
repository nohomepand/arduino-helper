#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>
#include <memory>
#include <utility>

#if defined(_MSVC_LANG)
#define ARDUINO_HELPER_CXX_STANDARD _MSVC_LANG
#else
#define ARDUINO_HELPER_CXX_STANDARD __cplusplus
#endif

#if ARDUINO_HELPER_CXX_STANDARD > 201103L
#define ARDUINO_HELPER_LIMITED_INT_STATIC_BOUNDS
#endif

/**
 * @brief 整数値を[min, max)の範囲に巡回させる整数ラッパー。
 *
 * 二項演算の結果は左辺の値域に折り返されます。比較演算は保持値を比較し、
 * 値域そのものは比較しません。maxは範囲に含まれません。
 * C++11ではLimitedInt<T> value(min, max)、C++14以降では
 * LimitedInt<T, min, max> value の形式で値域を指定します。
 * min < maxは必須条件で、除算・剰余の右辺に0を指定した場合の動作は未定義です。
 *
 * @tparam T 基底となる整数型（bool以外）。
 */
#ifdef ARDUINO_HELPER_LIMITED_INT_STATIC_BOUNDS
template <typename T,
          T Min = (std::numeric_limits<T>::min)(),
          T Max = (std::numeric_limits<T>::max)()>
#else
template <typename T>
#endif
class LimitedInt {
    static_assert(std::is_integral<T>::value, "LimitedInt requires an integral type");
    static_assert(!std::is_same<T, bool>::value, "LimitedInt does not support bool");
#ifdef ARDUINO_HELPER_LIMITED_INT_STATIC_BOUNDS
    static_assert(Min < Max, "LimitedInt requires Min < Max");
#endif

    using UnsignedT = typename std::make_unsigned<T>::type;

public:
#ifdef ARDUINO_HELPER_LIMITED_INT_STATIC_BOUNDS
    /** @brief テンプレート引数で指定された値域の下限を初期値として作る。 */
    LimitedInt() : _min(Min), _max(Max), _value(Min) {}

    /** @brief 指定した初期値をテンプレート引数の値域に折り返して作る。 */
    explicit LimitedInt(T initial) : _min(Min), _max(Max), _value(Min) {
        _value = wrap(initial);
    }
#else
    /** @brief minを初期値として[min, max)の値域を作る。 */
    LimitedInt(T min, T max) : _min(min), _max(max), _value(min) {
        assert(_min < _max);
    }
#endif

    /** @brief 保持値をTへ暗黙変換する。 */
    operator T() const { return _value; }

    /** @brief 保持値をTとして返す。 */
    T value() const { return _value; }

    /** @brief 指定値を値域に折り返して代入する。 */
    LimitedInt& operator=(T rhs) {
        _value = wrap(rhs);
        return *this;
    }

    /** @brief rhsを加算し、結果をこのオブジェクトの値域に折り返す。 */
    LimitedInt operator+(T rhs) const { return binary(rhs, Add); }
    /** @brief rhsを減算し、結果をこのオブジェクトの値域に折り返す。 */
    LimitedInt operator-(T rhs) const { return binary(rhs, Subtract); }
    /** @brief rhsを乗算し、結果をこのオブジェクトの値域に折り返す。 */
    LimitedInt operator*(T rhs) const { return binary(rhs, Multiply); }
    /** @brief rhsで除算し、結果をこのオブジェクトの値域に折り返す。 */
    LimitedInt operator/(T rhs) const { return binary(rhs, Divide); }
    /** @brief rhsで剰余を求め、結果をこのオブジェクトの値域に折り返す。 */
    LimitedInt operator%(T rhs) const { return binary(rhs, Remainder); }

    LimitedInt operator+(const LimitedInt& rhs) const { return *this + rhs._value; }
    LimitedInt operator-(const LimitedInt& rhs) const { return *this - rhs._value; }
    LimitedInt operator*(const LimitedInt& rhs) const { return *this * rhs._value; }
    LimitedInt operator/(const LimitedInt& rhs) const { return *this / rhs._value; }
    LimitedInt operator%(const LimitedInt& rhs) const { return *this % rhs._value; }

    /** @brief rhsを加算して保持値を更新する。 */
    LimitedInt& operator+=(T rhs) { return apply(rhs, Add); }
    /** @brief rhsを減算して保持値を更新する。 */
    LimitedInt& operator-=(T rhs) { return apply(rhs, Subtract); }
    /** @brief rhsを乗算して保持値を更新する。 */
    LimitedInt& operator*=(T rhs) { return apply(rhs, Multiply); }
    /** @brief rhsで除算して保持値を更新する。 */
    LimitedInt& operator/=(T rhs) { return apply(rhs, Divide); }
    /** @brief rhsで剰余を求めて保持値を更新する。 */
    LimitedInt& operator%=(T rhs) { return apply(rhs, Remainder); }

    LimitedInt& operator+=(const LimitedInt& rhs) { return *this += rhs._value; }
    LimitedInt& operator-=(const LimitedInt& rhs) { return *this -= rhs._value; }
    LimitedInt& operator*=(const LimitedInt& rhs) { return *this *= rhs._value; }
    LimitedInt& operator/=(const LimitedInt& rhs) { return *this /= rhs._value; }
    LimitedInt& operator%=(const LimitedInt& rhs) { return *this %= rhs._value; }

    bool operator==(T rhs) const { return _value == rhs; }
    bool operator!=(T rhs) const { return _value != rhs; }
    bool operator<(T rhs) const { return _value < rhs; }
    bool operator<=(T rhs) const { return _value <= rhs; }
    bool operator>(T rhs) const { return _value > rhs; }
    bool operator>=(T rhs) const { return _value >= rhs; }

    bool operator==(const LimitedInt& rhs) const { return _value == rhs._value; }
    bool operator!=(const LimitedInt& rhs) const { return _value != rhs._value; }
    bool operator<(const LimitedInt& rhs) const { return _value < rhs._value; }
    bool operator<=(const LimitedInt& rhs) const { return _value <= rhs._value; }
    bool operator>(const LimitedInt& rhs) const { return _value > rhs._value; }
    bool operator>=(const LimitedInt& rhs) const { return _value >= rhs._value; }

private:
    enum Operation { Add, Subtract, Multiply, Divide, Remainder };

    static std::uintmax_t magnitude(T value) {
        if (std::is_signed<T>::value && value < 0) {
            return static_cast<std::uintmax_t>(
                static_cast<UnsignedT>(-(value + 1)) + static_cast<UnsignedT>(1));
        }
        return static_cast<std::uintmax_t>(static_cast<UnsignedT>(value));
    }

    static std::uintmax_t residue(T value, std::uintmax_t modulus) {
        return signedResidue(magnitude(value),
                             std::is_signed<T>::value && value < 0, modulus);
    }

    static std::uintmax_t signedResidue(std::uintmax_t value, bool negative,
                                        std::uintmax_t modulus) {
        const std::uintmax_t result = value % modulus;
        if (negative && result != 0) {
            return modulus - result;
        }
        return result;
    }

    static std::uintmax_t addMod(std::uintmax_t lhs, std::uintmax_t rhs,
                                 std::uintmax_t modulus) {
        const std::uintmax_t distanceToWrap = modulus - rhs;
        return lhs >= distanceToWrap ? lhs - distanceToWrap : lhs + rhs;
    }

    static std::uintmax_t subtractMod(std::uintmax_t lhs, std::uintmax_t rhs,
                                      std::uintmax_t modulus) {
        return lhs >= rhs ? lhs - rhs : modulus - (rhs - lhs);
    }

    static std::uintmax_t multiplyMod(std::uintmax_t lhs, std::uintmax_t rhs,
                                      std::uintmax_t modulus) {
        std::uintmax_t result = 0;
        while (rhs != 0) {
            if ((rhs & 1U) != 0) {
                result = addMod(result, lhs, modulus);
            }
            rhs >>= 1;
            if (rhs != 0) {
                lhs = addMod(lhs, lhs, modulus);
            }
        }
        return result;
    }

    std::uintmax_t width() const {
        const UnsignedT difference =
            static_cast<UnsignedT>(_max) - static_cast<UnsignedT>(_min);
        return static_cast<std::uintmax_t>(difference);
    }

    T fromOffset(std::uintmax_t offset) const {
        if (!std::is_signed<T>::value) {
            return static_cast<T>(static_cast<UnsignedT>(_min) +
                                 static_cast<UnsignedT>(offset));
        }

        if (_min >= 0) {
            return static_cast<T>(_min + static_cast<T>(offset));
        }

        const std::uintmax_t minMagnitude = magnitude(_min);
        if (offset < minMagnitude) {
            const std::uintmax_t negativeMagnitude = minMagnitude - offset;
            const std::uintmax_t lowestMagnitude =
                magnitude((std::numeric_limits<T>::min)());
            if (negativeMagnitude == lowestMagnitude) {
                return (std::numeric_limits<T>::min)();
            }
            return static_cast<T>(-static_cast<T>(negativeMagnitude));
        }
        return static_cast<T>(offset - minMagnitude);
    }

    T wrap(T value) const {
        const std::uintmax_t range = width();
        const std::uintmax_t offset =
            subtractMod(residue(value, range), residue(_min, range), range);
        return fromOffset(offset);
    }

    std::uintmax_t resultResidue(T lhs, T rhs, Operation operation,
                                 std::uintmax_t range) const {
        const std::uintmax_t lhsResidue = residue(lhs, range);
        const std::uintmax_t rhsResidue = residue(rhs, range);
        const std::uintmax_t minResidue = residue(_min, range);
        std::uintmax_t result;

        switch (operation) {
        case Add:
            result = subtractMod(addMod(lhsResidue, rhsResidue, range),
                                 minResidue, range);
            break;
        case Subtract:
            result = subtractMod(subtractMod(lhsResidue, rhsResidue, range),
                                 minResidue, range);
            break;
        case Multiply:
            result = subtractMod(multiplyMod(lhsResidue, rhsResidue, range),
                                 minResidue, range);
            break;
        case Divide: {
            const std::uintmax_t quotient = magnitude(lhs) / magnitude(rhs);
            const bool negative =
                std::is_signed<T>::value && ((lhs < 0) != (rhs < 0));
            const std::uintmax_t quotientResidue =
                signedResidue(quotient, negative, range);
            result = subtractMod(quotientResidue, minResidue, range);
            break;
        }
        case Remainder: {
            const std::uintmax_t remainder = magnitude(lhs) % magnitude(rhs);
            const bool negative = std::is_signed<T>::value && lhs < 0;
            const std::uintmax_t remainderResidue =
                signedResidue(remainder, negative, range);
            result = subtractMod(remainderResidue, minResidue, range);
            break;
        }
        }
        return result;
    }

    LimitedInt binary(T rhs, Operation operation) const {
        LimitedInt result(*this);
        const std::uintmax_t range = width();
        const std::uintmax_t offset = resultResidue(_value, rhs, operation, range);
        result._value = fromOffset(offset);
        return result;
    }

    LimitedInt& apply(T rhs, Operation operation) {
        const std::uintmax_t range = width();
        const std::uintmax_t offset = resultResidue(_value, rhs, operation, range);
        _value = fromOffset(offset);
        return *this;
    }

    T _min;
    T _max;
    T _value;
};

#ifdef ARDUINO_HELPER_LIMITED_INT_STATIC_BOUNDS
#undef ARDUINO_HELPER_LIMITED_INT_STATIC_BOUNDS
#endif

/**
 * @brief 固定容量のFIFOリングバッファ。
 *
 * バッファ領域は生成時に確保し、要素オブジェクトは格納時に構築します。
 * 取り出し、purge()、またはRingBufferの破棄時に、格納中の要素を破棄します。
 * コピーはできませんが、ムーブできます。スレッドセーフではありません。
 *
 * TにはC配列型ではなく、std::arrayなどの値型を指定してください。
 *
 * @tparam T 格納する要素型。emplace_back()の引数で構築可能で、
 *           pop_front()で出力先へムーブ代入可能であること。
 */
template <typename T>
class RingBuffer {
public:
    /**
     * @brief 指定容量のバッファを作る。
     * @param capacity 格納可能な要素数。1以上を指定すること。
     */
    explicit RingBuffer(std::size_t capacity)
        : _allocator(),
          _buffer(capacity == 0 ? nullptr : _allocator.allocate(capacity)),
          _capacity(capacity),
          _write(0), _read(0), _count(0) {
        assert(capacity > 0 && "RingBuffer capacity must be greater than 0");
    }

    /**
     * @brief 格納中の要素を破棄し、バッファ領域を解放する。
     */
    ~RingBuffer() {
        purge();
        if (_buffer != nullptr) {
            _allocator.deallocate(_buffer, _capacity);
        }
    }

    /** @brief コピー構築を禁止する。 */
    RingBuffer(const RingBuffer&) = delete;

    /** @brief コピー代入を禁止する。 */
    RingBuffer& operator=(const RingBuffer&) = delete;

    /**
     * @brief バッファの所有権を移す。
     * @param other 移動元。移動後の容量と要素数は0になる。
     */
    RingBuffer(RingBuffer&& other) noexcept
        : _allocator(),
          _buffer(other._buffer),
          _capacity(other._capacity), _write(other._write),
          _read(other._read), _count(other._count) {
        other._buffer = nullptr;
        other.reset();
    }

    /**
     * @brief バッファの所有権を移す。代入先の格納要素は先に破棄する。
     * @param other 移動元。移動後の容量と要素数は0になる。
     */
    RingBuffer& operator=(RingBuffer&& other) noexcept {
        if (this != &other) {
            purge();
            if (_buffer != nullptr) {
                _allocator.deallocate(_buffer, _capacity);
            }
            _buffer = other._buffer;
            _capacity = other._capacity;
            _write = other._write;
            _read = other._read;
            _count = other._count;
            other._buffer = nullptr;
            other.reset();
        }
        return *this;
    }

    /**
     * @brief 要素をコピーして末尾に追加する。
     * @param value 追加する値。
     * @return 追加できた場合はtrue。満杯ならfalse。
     */
    bool push_back(const T& value) {
        return emplace_back(value);
    }

    /**
     * @brief 要素をムーブして末尾に追加する。
     * @param value 追加する値。
     * @return 追加できた場合はtrue。満杯ならfalse。
     */
    bool push_back(T&& value) {
        return emplace_back(std::move(value));
    }

    /**
     * @brief 引数から要素を構築して末尾に追加する。
     * @tparam Args 要素のコンストラクター引数型。
     * @param args 要素のコンストラクター引数。
     * @return 追加できた場合はtrue。満杯ならfalse。
     */
    template <typename... Args>
    bool emplace_back(Args&&... args) {
        if (_count == _capacity) return false;
        std::allocator_traits<std::allocator<T>>::construct(
            _allocator, _buffer + _write, std::forward<Args>(args)...);
        _write = next(_write);
        ++_count;
        return true;
    }

    /**
     * @brief 先頭要素を出力先へムーブし、格納要素を破棄する。
     * @param value 取り出した値の代入先。
     * @return 取り出せた場合はtrue。空ならfalse。
     */
    bool pop_front(T& value) {
        if (_count == 0) return false;
        T* item = _buffer + _read;
        value = std::move(*item);
        std::allocator_traits<std::allocator<T>>::destroy(_allocator, item);
        _read = next(_read);
        --_count;
        return true;
    }

    /** @brief 格納可能な要素数を返す。 */
    std::size_t capacity() const {
        return _capacity;
    }

    /** @brief 現在格納されている要素数を返す。 */
    std::size_t size() const {
        return _count;
    }

    /** @brief バッファに要素がある場合にtrueへ変換する。 */
    operator bool() const {
        return _count != 0;
    }

    /**
     * @brief 格納中の全要素をただちに破棄し、バッファを空にする。
     * @note 領域自体は解放せず、以後の追加に再利用する。
     */
    void purge() {
        while (_count != 0) {
            std::allocator_traits<std::allocator<T>>::destroy(
                _allocator, _buffer + _read);
            _read = next(_read);
            --_count;
        }
        _write = _read = 0;
    }

private:
    std::size_t next(std::size_t index) const {
        return index + 1 == _capacity ? 0 : index + 1;
    }

    void reset() {
        _capacity = _write = _read = _count = 0;
    }

    std::allocator<T> _allocator;
    T* _buffer;
    std::size_t _capacity;
    std::size_t _write;
    std::size_t _read;
    std::size_t _count;
};

#undef ARDUINO_HELPER_CXX_STANDARD
#ifdef ARDUINO_HELPER_LIMITED_INT_STATIC_BOUNDS
#undef ARDUINO_HELPER_LIMITED_INT_STATIC_BOUNDS
#endif

#pragma once

#include <Arduino.h>
#include <Preferences.h>

/**
 * @brief ESP32 Preferencesの値をプロパティ風に扱うラッパー。
 *
 * 通常の固定サイズ型はオブジェクト表現をそのままNVSへ保存する。
 * Stringは下の特殊化で文字列として保存する。
 */
template<typename T>
class NvsProperty {
private:
    T value;
    const char* ns;
    const char* key;
public:
    /**
     * @brief NVSの値を読み込む。キーがなければdefault_valueをRAM上に設定する。
     * @note namespace_nameとkey_nameは、このプロパティの使用中有効な文字列にする。
     */
    void init(const char* namespace_name, const char* key_name, T default_value) {
        ns = namespace_name;
        key = key_name;
        Preferences prefs;
        prefs.begin(ns, true);
        if (prefs.isKey(key)) {
            prefs.getBytes(key, &value, sizeof(T));
        } else {
            value = default_value;
        }
        prefs.end();
    }

    /** 現在の値をTへ変換して返す。 */
    operator T() const { return value; }

    /** 値を更新し、NVSへ保存する。 */
    NvsProperty& operator=(const T& newValue) {
        value = newValue;
        Preferences prefs;
        prefs.begin(ns, false);
        prefs.putBytes(key, &value, sizeof(T));
        prefs.end();
        return *this;
    }
};

/** StringをPreferencesの文字列APIで保存する特殊化。 */
template<>
class NvsProperty<String> {
private:
    String value;
    const char* ns;
    const char* key;
public:
    /**
     * @brief NVSの文字列を読み込む。キーがなければdefault_valueをRAM上に設定する。
     * @note namespace_nameとkey_nameは、このプロパティの使用中有効な文字列にする。
     */
    void init(const char* namespace_name, const char* key_name, String default_value) {
        ns = namespace_name;
        key = key_name;
        Preferences prefs;
        prefs.begin(ns, true);
        if (prefs.isKey(key)) {
            value = prefs.getString(key, default_value);
        } else {
            value = default_value;
        }
        prefs.end();
    }

    /** 現在の値をStringへ変換して返す。 */
    operator String() const { return value; }

    /** 内部StringのC文字列ポインターを返す。 */
    const char* c_str() const { return value.c_str(); }

    /** 値を更新し、NVSへ保存する。 */
    NvsProperty& operator=(const String& newValue) {
        value = newValue;
        Preferences prefs;
        prefs.begin(ns, false);
        prefs.putString(key, value);
        prefs.end();
        return *this;
    }
};

/** NvsPropertyメンバーを宣言する。init()は別途呼び出すこと。 */
#define DEFINE_NVS_PROP(Type, Name) NvsProperty<Type> Name

static void clearPreference(const char* ns) {
    Preferences prefs;
    prefs.begin(ns, true);
    prefs.clear();
    prefs.end();
}

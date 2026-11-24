#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <functional>
#include <vector>
#include <map>

// コマンド関数の型を定義（読みやすさのためエイリアス化）
using CommandHandler = std::function<void(const String&)>;

/**
 * @brief Streamから行単位でコマンドを読み取り、登録済みハンドラーを呼び出す。
 *
 * コマンドと引数は最初の空白で分割する。入力処理はfeedInput()を呼んだ
 * タスク上で同期的に実行される。
 */
class CLIHelper {
public:
    /**
     * @param prompt 行処理後に表示するプロンプト。空文字列なら表示しない。
     * @param echo trueの場合、入力文字をStreamへエコーバックする。
     * @param stream 入出力に使うStream。省略時はSerial。
     * @param max_buf 入力行の最大バイト数。超過した入力は無視される。
     */
    CLIHelper(const String& prompt = "> ", bool echo = false, Stream& stream = Serial, size_t max_buf = 512)
        : _prompt(prompt), _echo(echo), _stream(&stream), _max_buf(max_buf) {}

    /** 入力バッファを準備し、初期プロンプトを表示する。 */
    void begin() {
        _buffer.reserve(128);
        if (_prompt.length() > 0) {
            _stream->print(_prompt);
        }
    }

    /** 入出力先を切り替える。新しいStreamはCLIHelperより長く有効である必要がある。 */
    void setStream(Stream& stream) {
        _stream = &stream;
    }

    void setEcho(bool echo) {
        _echo = echo;
    }

    void setMaxBuf(size_t max_buf) {
        _max_buf = max_buf;
    }

    /**
     * コマンドを登録する。同じ名前を再登録すると、既存のハンドラーを置き換える。
     * short_helpを省略すると "Call <command>" がヘルプ文字列に使われる。
     */
    void registerHandler(const String& command, CommandHandler handler, String short_help = "") {
        if (short_help.length() == 0)
            short_help = "Call " + command;
        _handler_map[command] = {handler, short_help};
    }

    /** 登録済みコマンドを "name: help" の行形式で返す。 */
    String getHelpString() const {
        String tmp = "";
        for (const auto& kv : _handler_map) {
            tmp += kv.first + ": " + kv.second.second + "\n";
        }
        return tmp;
    }

    /** 未登録コマンドを報告し、登録済みコマンドのヘルプをStreamへ出力する。 */
    void unhandledCommand(const String& command, const String& args) const {
        _stream->println(getHelpString());
        _stream->println("====");
        _stream->printf("Unknown command [%s] args=[%s]\n", command.c_str(), args.c_str());
    }

    void feed(const String& line) {
        String command, args;
        int spaceIndex = line.indexOf(' ');
        if (spaceIndex == -1) {
            command = line;
            args = "";
        } else {
            command = line.substring(0, spaceIndex);
            args = line.substring(spaceIndex + 1);
        }

        auto found = _handler_map.find(command);
        if (found != _handler_map.end()) {
            found->second.first(args);
        } else {
            unhandledCommand(command, args);
        }
    }

    /**
     * 利用可能な入力を読み取り、改行でコマンドを実行する。
     * Arduinoのloop()などから繰り返し呼び出す。
     */
    void feedInput() {
        if (!_stream) return;

        while (_stream->available() > 0) {
            int ch = _stream->read();
            if (ch == -1) {
                break;
            } else if (ch == '\r') {
                continue;
            } else if (ch == '\b' || ch == 0x7F) {
                if (!_buffer.empty()) {
                    popUtf8Char(_buffer);
                    if (_echo)
                        _stream->print("\b \b");
                }
            } else if (ch != '\n') {
                if (_buffer.size() < _max_buf && (ch >= 0x20 || (ch & 0x80))) {
                    _buffer.push_back((uint8_t)ch);
                    if (_echo)
                        _stream->write((uint8_t)ch);
                }
            } else {
                if (_echo)
                    _stream->println();

                if (!_buffer.empty()) {
                    _buffer.push_back('\0');
                    String tmp((const char*)_buffer.data());
                    _buffer.clear();

                    feed(tmp);
                }

                if (_prompt.length() > 0)
                    _stream->print(_prompt);
            }
        }
    }

private:
    const String _prompt;
    bool _echo;
    Stream* _stream; // ポインタで持つことで後から差し替えも可能になる
    size_t _max_buf;
    std::vector<uint8_t> _buffer;
    std::map<String, std::pair<CommandHandler, String>> _handler_map;

    static void popUtf8Char(std::vector<uint8_t>& buf) { // バッファからUTF-8の1文字分（1〜4バイト）を削除する関数
        if (buf.empty()) return;
        while (!buf.empty() && (buf.back() & 0xC0) == 0x80) {
            buf.pop_back();
        }
        if (!buf.empty()) {
            buf.pop_back();
        }
    }
};

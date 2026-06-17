#include "json.h"

#include <windows.h>

#include <fstream>
#include <iterator>

namespace wingnome {

std::optional<std::wstring> JsonValue::getString(const std::wstring& key) const {
    if (type_ != Type::Object) return std::nullopt;
    auto it = obj_.find(key);
    if (it == obj_.end() || it->second.type_ != Type::String) return std::nullopt;
    return it->second.str_;
}

std::optional<double> JsonValue::getNumber(const std::wstring& key) const {
    if (type_ != Type::Object) return std::nullopt;
    auto it = obj_.find(key);
    if (it == obj_.end()) return std::nullopt;
    if (it->second.type_ == Type::Number) return it->second.num_;
    return std::nullopt;
}

std::optional<JsonArray> JsonValue::getArray(const std::wstring& key) const {
    if (type_ != Type::Object) return std::nullopt;
    auto it = obj_.find(key);
    if (it == obj_.end() || it->second.type_ != Type::Array) return std::nullopt;
    return it->second.arr_;
}

std::optional<JsonObject> JsonValue::getObject(const std::wstring& key) const {
    if (type_ != Type::Object) return std::nullopt;
    auto it = obj_.find(key);
    if (it == obj_.end() || it->second.type_ != Type::Object) return std::nullopt;
    return it->second.obj_;
}

std::optional<JsonValue> JsonParser::parseFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::wstring text = utf8ToWide(bytes);
    JsonParser parser{std::move(text), 0};
    auto value = parser.parseValue();
    parser.skipWs();
    if (!value || parser.pos_ != parser.text_.size()) return std::nullopt;
    return value;
}

JsonParser::JsonParser(std::wstring text, size_t pos)
    : text_(std::move(text)), pos_(pos) {}

std::wstring JsonParser::utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), out.data(), len);
    return out;
}

wchar_t JsonParser::peek() const { return pos_ < text_.size() ? text_[pos_] : L'\0'; }

void JsonParser::skipWs() {
    while (pos_ < text_.size() && iswspace(text_[pos_])) ++pos_;
}

bool JsonParser::consume(wchar_t c) {
    skipWs();
    if (peek() == c) {
        ++pos_;
        return true;
    }
    return false;
}

std::optional<std::wstring> JsonParser::parseString() {
    skipWs();
    if (peek() != L'"') return std::nullopt;
    ++pos_;
    std::wstring out;
    while (pos_ < text_.size()) {
        wchar_t c = text_[pos_++];
        if (c == L'"') return out;
        if (c == L'\\' && pos_ < text_.size()) {
            wchar_t e = text_[pos_++];
            switch (e) {
                case L'"': out += L'"'; break;
                case L'\\': out += L'\\'; break;
                case L'/': out += L'/'; break;
                case L'b': out += L'\b'; break;
                case L'f': out += L'\f'; break;
                case L'n': out += L'\n'; break;
                case L'r': out += L'\r'; break;
                case L't': out += L'\t'; break;
                case L'u': {
                    if (pos_ + 4 > text_.size()) return std::nullopt;
                    unsigned code = 0;
                    for (int i = 0; i < 4; ++i) {
                        wchar_t h = text_[pos_++];
                        code <<= 4;
                        if (h >= L'0' && h <= L'9') code |= h - L'0';
                        else if (h >= L'a' && h <= L'f') code |= h - L'a' + 10;
                        else if (h >= L'A' && h <= L'F') code |= h - L'A' + 10;
                        else return std::nullopt;
                    }
                    out += static_cast<wchar_t>(code);
                    break;
                }
                default: out += e; break;
            }
        } else {
            out += c;
        }
    }
    return std::nullopt;
}

std::optional<double> JsonParser::parseNumber() {
    skipWs();
    const size_t start = pos_;
    if (peek() == L'-') ++pos_;
    while (pos_ < text_.size() && (iswdigit(text_[pos_]) || text_[pos_] == L'.')) ++pos_;
    if (pos_ == start || (pos_ == start + 1 && text_[start] == L'-')) return std::nullopt;
    return std::wcstod(text_.c_str() + start, nullptr);
}

std::optional<JsonValue> JsonParser::parseValue() {
    skipWs();
    const wchar_t c = peek();
    if (c == L'"') {
        auto s = parseString();
        if (!s) return std::nullopt;
        return JsonValue(*s);
    }
    if (c == L'{') return parseObject();
    if (c == L'[') return parseArray();
    if (c == L'-' || iswdigit(c)) {
        auto n = parseNumber();
        if (!n) return std::nullopt;
        return JsonValue(*n);
    }
    return std::nullopt;
}

std::optional<JsonValue> JsonParser::parseObject() {
    if (!consume(L'{')) return std::nullopt;
    JsonObject obj;
    skipWs();
    if (consume(L'}')) return JsonValue(std::move(obj));
    while (true) {
        auto key = parseString();
        if (!key || !consume(L':')) return std::nullopt;
        auto val = parseValue();
        if (!val) return std::nullopt;
        obj.emplace(*key, *val);
        skipWs();
        if (consume(L'}')) return JsonValue(std::move(obj));
        if (!consume(L',')) return std::nullopt;
    }
}

std::optional<JsonValue> JsonParser::parseArray() {
    if (!consume(L'[')) return std::nullopt;
    JsonArray arr;
    skipWs();
    if (consume(L']')) return JsonValue(std::move(arr));
    while (true) {
        auto val = parseValue();
        if (!val) return std::nullopt;
        arr.push_back(*val);
        skipWs();
        if (consume(L']')) return JsonValue(std::move(arr));
        if (!consume(L',')) return std::nullopt;
    }
}

}  // namespace wingnome

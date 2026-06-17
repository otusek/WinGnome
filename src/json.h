#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace wingnome {

class JsonValue;

using JsonObject = std::unordered_map<std::wstring, JsonValue>;
using JsonArray = std::vector<JsonValue>;

class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    JsonValue() = default;
    explicit JsonValue(std::wstring s) : type_(Type::String), str_(std::move(s)) {}
    explicit JsonValue(double n) : type_(Type::Number), num_(n) {}
    explicit JsonValue(JsonArray a) : type_(Type::Array), arr_(std::move(a)) {}
    explicit JsonValue(JsonObject o) : type_(Type::Object), obj_(std::move(o)) {}

    Type type() const { return type_; }

    const std::wstring& asString() const { return str_; }
    double asNumber() const { return num_; }
    const JsonArray& asArray() const { return arr_; }
    const JsonObject& asObject() const { return obj_; }

    std::optional<std::wstring> getString(const std::wstring& key) const;
    std::optional<double> getNumber(const std::wstring& key) const;
    std::optional<JsonArray> getArray(const std::wstring& key) const;
    std::optional<JsonObject> getObject(const std::wstring& key) const;

private:
    Type type_{Type::Null};
    std::wstring str_;
    double num_{0};
    JsonArray arr_;
    JsonObject obj_;

    friend class JsonParser;
};

class JsonParser {
public:
    static std::optional<JsonValue> parseFile(const std::filesystem::path& path);

private:
    std::wstring text_;
    size_t pos_{0};

    JsonParser(std::wstring text, size_t pos);

    static std::wstring utf8ToWide(const std::string& s);
    wchar_t peek() const;
    void skipWs();
    bool consume(wchar_t c);
    std::optional<std::wstring> parseString();
    std::optional<double> parseNumber();
    std::optional<JsonValue> parseValue();
    std::optional<JsonValue> parseObject();
    std::optional<JsonValue> parseArray();
};

}  // namespace wingnome

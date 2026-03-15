#include "core/json_value.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace archscope::core {

namespace {

class JsonParser {
  public:
    explicit JsonParser(std::string text) : text_(std::move(text)) {}

    JsonValue parse() {
        skip_whitespace();
        JsonValue value = parse_value();
        skip_whitespace();
        if (!at_end()) {
            throw std::runtime_error("invalid trailing content");
        }
        return value;
    }

  private:
    JsonValue parse_value() {
        skip_whitespace();
        if (at_end()) {
            throw std::runtime_error("unexpected end of input");
        }

        const char current = text_[position_];
        if (current == '"') {
            return JsonValue{parse_string()};
        }
        if (current == '[') {
            return JsonValue{parse_array()};
        }
        if (current == '{') {
            return JsonValue{parse_object()};
        }

        throw std::runtime_error("unsupported json token");
    }

    JsonArray parse_array() {
        expect('[');
        skip_whitespace();

        JsonArray values;
        if (try_consume(']')) {
            return values;
        }

        while (true) {
            values.push_back(parse_value());
            skip_whitespace();
            if (try_consume(']')) {
                break;
            }
            expect(',');
        }

        return values;
    }

    JsonObject parse_object() {
        expect('{');
        skip_whitespace();

        JsonObject values;
        if (try_consume('}')) {
            return values;
        }

        while (true) {
            const std::string key = parse_string();
            skip_whitespace();
            expect(':');
            JsonValue value = parse_value();
            values.emplace(key, std::move(value));
            skip_whitespace();
            if (try_consume('}')) {
                break;
            }
            expect(',');
        }

        return values;
    }

    std::string parse_string() {
        expect('"');

        std::string value;
        while (!at_end()) {
            const char current = text_[position_++];
            if (current == '"') {
                return value;
            }
            if (current == '\\') {
                if (at_end()) {
                    throw std::runtime_error("invalid escape sequence");
                }
                value.push_back(parse_escape());
                continue;
            }
            value.push_back(current);
        }

        throw std::runtime_error("unterminated string");
    }

    char parse_escape() {
        const char escaped = text_[position_++];
        switch (escaped) {
        case '"':
        case '\\':
        case '/':
            return escaped;
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        default:
            throw std::runtime_error("unsupported json escape");
        }
    }

    void skip_whitespace() {
        while (!at_end() && std::isspace(static_cast<unsigned char>(
                                text_[position_])) != 0) {
            ++position_;
        }
    }

    void expect(char expected) {
        skip_whitespace();
        if (at_end() || text_[position_] != expected) {
            throw std::runtime_error("invalid json structure");
        }
        ++position_;
    }

    bool try_consume(char expected) {
        skip_whitespace();
        if (!at_end() && text_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    [[nodiscard]] bool at_end() const { return position_ >= text_.size(); }

    std::string text_;
    std::size_t position_ = 0;
};

} // namespace

JsonValue parse_json(std::string_view text) {
    return JsonParser(std::string(text)).parse();
}

const JsonArray &as_array(const JsonValue &value) {
    const auto *array = std::get_if<JsonArray>(&value.storage);
    if (array == nullptr) {
        throw std::runtime_error("expected json array");
    }
    return *array;
}

const JsonObject &as_object(const JsonValue &value) {
    const auto *object = std::get_if<JsonObject>(&value.storage);
    if (object == nullptr) {
        throw std::runtime_error("expected json object");
    }
    return *object;
}

const std::string &as_string(const JsonValue &value) {
    const auto *string_value = std::get_if<std::string>(&value.storage);
    if (string_value == nullptr) {
        throw std::runtime_error("expected json string");
    }
    return *string_value;
}

const JsonValue &required_field(const JsonObject &object, const char *key) {
    const auto iterator = object.find(key);
    if (iterator == object.end()) {
        throw std::runtime_error(std::string("missing required field: ") +
                                 key);
    }
    return iterator->second;
}

} // namespace archscope::core

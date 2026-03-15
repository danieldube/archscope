#pragma once

#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace archscope::core {

struct JsonValue;

using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

struct JsonValue {
  using Storage = std::variant<std::string, JsonArray, JsonObject>;

  Storage storage;
};

JsonValue parse_json(std::string_view text);

const JsonArray &as_array(const JsonValue &value);
const JsonObject &as_object(const JsonValue &value);
const std::string &as_string(const JsonValue &value);
const JsonValue &required_field(const JsonObject &object, const char *key);

} // namespace archscope::core

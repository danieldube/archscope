#include "core/compilation_database.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>

namespace archscope::core {

namespace {

struct JsonValue;

using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue>;

struct JsonValue {
  using Storage = std::variant<std::string, JsonArray, JsonObject>;

  Storage storage;
};

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
    while (!at_end() &&
           std::isspace(static_cast<unsigned char>(text_[position_])) != 0) {
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
    throw std::runtime_error(std::string("missing required field: ") + key);
  }
  return iterator->second;
}

std::vector<std::string> split_command_arguments(const std::string &command) {
  std::vector<std::string> arguments;
  std::string current;
  bool in_single_quotes = false;
  bool in_double_quotes = false;
  bool escaped = false;

  for (const char current_char : command) {
    if (escaped) {
      current.push_back(current_char);
      escaped = false;
      continue;
    }

    if (current_char == '\\' && !in_single_quotes) {
      escaped = true;
      continue;
    }

    if (current_char == '\'' && !in_double_quotes) {
      in_single_quotes = !in_single_quotes;
      continue;
    }

    if (current_char == '"' && !in_single_quotes) {
      in_double_quotes = !in_double_quotes;
      continue;
    }

    if (!in_single_quotes && !in_double_quotes &&
        std::isspace(static_cast<unsigned char>(current_char)) != 0) {
      if (!current.empty()) {
        arguments.push_back(current);
        current.clear();
      }
      continue;
    }

    current.push_back(current_char);
  }

  if (escaped || in_single_quotes || in_double_quotes) {
    throw std::runtime_error("invalid command string");
  }

  if (!current.empty()) {
    arguments.push_back(current);
  }

  return arguments;
}

CompilationDatabaseEntry parse_entry(const JsonValue &entry_value) {
  const JsonObject &entry = as_object(entry_value);
  CompilationDatabaseEntry parsed_entry{
      as_string(required_field(entry, "file")),
      {},
      as_string(required_field(entry, "directory")),
      "",
  };

  const auto arguments_iterator = entry.find("arguments");
  if (arguments_iterator != entry.end()) {
    for (const JsonValue &argument : as_array(arguments_iterator->second)) {
      parsed_entry.arguments.push_back(as_string(argument));
    }
    return parsed_entry;
  }

  const auto command_iterator = entry.find("command");
  if (command_iterator == entry.end()) {
    throw std::runtime_error("missing required field: arguments or command");
  }

  parsed_entry.arguments =
      split_command_arguments(as_string(command_iterator->second));
  return parsed_entry;
}

} // namespace

namespace {

std::string
normalize_working_directory(const std::filesystem::path &database_path,
                            const std::string &working_directory) {
  const std::filesystem::path raw_path(working_directory);
  if (raw_path.is_absolute()) {
    return raw_path.lexically_normal().string();
  }

  const std::filesystem::path base_directory = database_path.parent_path();
  return (base_directory / raw_path).lexically_normal().string();
}

std::optional<std::string> optional_string_field(const JsonObject &object,
                                                 const char *key) {
  const auto iterator = object.find(key);
  if (iterator == object.end()) {
    return std::nullopt;
  }
  return as_string(iterator->second);
}

std::optional<std::string>
extract_output_argument(const std::vector<std::string> &arguments) {
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    const std::string &argument = arguments[index];
    if (argument == "-o") {
      if (index + 1U < arguments.size()) {
        return arguments[index + 1U];
      }
      return std::nullopt;
    }

    if (argument.rfind("-o", 0) == 0 && argument.size() > 2U) {
      return argument.substr(2);
    }
  }

  return std::nullopt;
}

std::string normalize_output_path(const CompilationDatabaseEntry &entry,
                                  const std::string &output_path) {
  const std::filesystem::path raw_path(output_path);
  if (raw_path.is_absolute()) {
    return raw_path.lexically_normal().string();
  }

  return (std::filesystem::path(entry.working_directory) / raw_path)
      .lexically_normal()
      .string();
}

std::string
derive_target_from_output_path(const std::filesystem::path &output_path) {
  auto component = output_path.begin();
  while (component != output_path.end()) {
    if (component->string() == "CMakeFiles") {
      const auto target_component = std::next(component);
      if (target_component != output_path.end()) {
        const std::string target_dir = target_component->string();
        constexpr std::string_view suffix{".dir"};
        if (target_dir.size() > suffix.size() &&
            target_dir.compare(target_dir.size() - suffix.size(), suffix.size(),
                               suffix) == 0) {
          return target_dir.substr(0, target_dir.size() - suffix.size());
        }
      }
    }
    ++component;
  }

  const std::filesystem::path parent = output_path.parent_path();
  if (!parent.empty()) {
    return parent.lexically_normal().string();
  }

  return output_path.lexically_normal().string();
}

std::string derive_compilation_target(const JsonObject &entry_object,
                                      const CompilationDatabaseEntry &entry) {
  std::optional<std::string> output_path =
      optional_string_field(entry_object, "output");
  if (!output_path.has_value()) {
    output_path = extract_output_argument(entry.arguments);
  }

  if (!output_path.has_value() || output_path->empty()) {
    return entry.source_path;
  }

  return derive_target_from_output_path(
      normalize_output_path(entry, *output_path));
}

} // namespace

std::vector<std::string> CompilationDatabase::translation_unit_paths() const {
  std::vector<std::string> paths;
  paths.reserve(entries.size());

  for (const CompilationDatabaseEntry &entry : entries) {
    paths.push_back(entry.source_path);
  }

  std::sort(paths.begin(), paths.end());
  return paths;
}

Result<CompilationDatabase>
load_compilation_database(const std::filesystem::path &path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    return Result<CompilationDatabase>::failure(
        {CompilationDatabaseErrorCode::file_not_found,
         "missing compile_commands.json file", path.string()});
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (input.bad()) {
    return Result<CompilationDatabase>::failure(
        {CompilationDatabaseErrorCode::io_error,
         "failed to read compile_commands.json file", path.string()});
  }

  try {
    CompilationDatabase database;
    const JsonValue root = JsonParser(buffer.str()).parse();
    for (const JsonValue &entry : as_array(root)) {
      const JsonObject &entry_object = as_object(entry);
      CompilationDatabaseEntry parsed_entry = parse_entry(entry);
      parsed_entry.working_directory =
          normalize_working_directory(path, parsed_entry.working_directory);
      parsed_entry.compilation_target =
          derive_compilation_target(entry_object, parsed_entry);
      database.entries.push_back(std::move(parsed_entry));
    }
    return Result<CompilationDatabase>::success(std::move(database));
  } catch (const std::runtime_error &error) {
    return Result<CompilationDatabase>::failure(
        {CompilationDatabaseErrorCode::invalid_format,
         std::string("invalid compile_commands.json: ") + error.what(),
         path.string()});
  }
}

} // namespace archscope::core

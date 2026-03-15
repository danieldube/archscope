#include "core/compile_commands_decoder.hpp"

#include <cctype>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace archscope::core {

namespace {

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

CompilationDatabase
decode_compile_commands(const JsonValue &root,
                        const std::filesystem::path &database_path) {
  CompilationDatabase database;
  for (const JsonValue &entry : as_array(root)) {
    const JsonObject &entry_object = as_object(entry);
    CompilationDatabaseEntry parsed_entry = parse_entry(entry);
    parsed_entry.working_directory = normalize_working_directory(
        database_path, parsed_entry.working_directory);
    parsed_entry.compilation_target =
        derive_compilation_target(entry_object, parsed_entry);
    database.entries.push_back(std::move(parsed_entry));
  }

  return database;
}

} // namespace archscope::core

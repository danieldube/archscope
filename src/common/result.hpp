#pragma once

#include <optional>
#include <utility>

namespace archscope::common {

template <typename TValue, typename TError> class Result {
public:
  static Result success(TValue value) { return Result(std::move(value)); }

  static Result failure(TError error) { return Result(std::move(error)); }

  [[nodiscard]] bool has_value() const { return value_.has_value(); }

  [[nodiscard]] const TValue &value() const { return *value_; }

  [[nodiscard]] const TError &error() const { return *error_; }

private:
  explicit Result(TValue value) : value_(std::move(value)) {}

  explicit Result(TError error) : error_(std::move(error)) {}

  std::optional<TValue> value_;
  std::optional<TError> error_;
};

} // namespace archscope::common

#pragma once

namespace platform::api {

struct Request {};

struct Response {};

struct Port {
  virtual ~Port() = default;
  virtual Response handle(const Request &request) = 0;
};

} // namespace platform::api

#include "../../include/platform/api.hpp"
#include "../app/facade.cpp"

namespace platform::adapters {

struct HttpController final : app::AppPort {
  app::AppRunner runner;
  api::Request request;

  void boot() override {}
};

} // namespace platform::adapters

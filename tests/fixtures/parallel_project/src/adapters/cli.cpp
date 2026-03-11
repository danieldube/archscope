#include "../app/facade.cpp"

namespace platform::adapters {

struct CliController final : app::AppPort {
  app::AppRunner runner;

  void boot() override {}
};

} // namespace platform::adapters

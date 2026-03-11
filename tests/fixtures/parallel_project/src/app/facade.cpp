#include "../domain/order.cpp"
#include "../domain/session.cpp"
#include "../domain/user.cpp"

namespace platform::app {

struct AppPort {
  virtual ~AppPort() = default;
  virtual void boot() = 0;
};

struct AppFacade {
  domain::OrderService order;
  domain::SessionService session;
  domain::UserService user;
};

struct AppRunner final : AppPort {
  AppFacade facade;

  void boot() override {}
};

} // namespace platform::app

#include "../../include/platform/api.hpp"
#include "../../include/platform/model.hpp"

namespace platform::domain {

struct UserService final : api::Port {
  model::UserRecord user;

  api::Response handle(const api::Request &request) override;
};

api::Response UserService::handle(const api::Request &request) {
  (void)request;
  return {};
}

} // namespace platform::domain

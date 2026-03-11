#include "../../include/platform/api.hpp"
#include "../../include/platform/model.hpp"

namespace platform::domain {

struct SessionService final : api::Port {
  model::SessionRecord session;

  api::Response handle(const api::Request &request) override;
};

api::Response SessionService::handle(const api::Request &request) {
  (void)request;
  return {};
}

} // namespace platform::domain

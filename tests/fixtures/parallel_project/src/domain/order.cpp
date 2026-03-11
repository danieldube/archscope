#include "../../include/platform/api.hpp"
#include "../../include/platform/model.hpp"

namespace platform::domain {

struct OrderService final : api::Port {
  model::OrderRecord order;

  api::Response handle(const api::Request &request) override;
};

api::Response OrderService::handle(const api::Request &request) {
  (void)request;
  return {};
}

} // namespace platform::domain

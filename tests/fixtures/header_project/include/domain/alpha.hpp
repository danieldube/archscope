#pragma once

namespace domain {

struct AlphaInterface {
  virtual ~AlphaInterface() = default;
  virtual void run() = 0;
};

} // namespace domain

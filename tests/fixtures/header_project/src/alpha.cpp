#include "../include/domain/alpha.hpp"
#include "../include/domain/beta.hpp"

namespace domain {

struct AlphaConcrete : AlphaInterface {
  BetaValue dependency;

  void run() override {}
};

} // namespace domain

#include "../include/shared.hpp"

struct Alpha final : SharedPort {
  SharedPort *member = nullptr;

  SharedPort *make(SharedPort *value) { return value; }

  void run() override {}
};

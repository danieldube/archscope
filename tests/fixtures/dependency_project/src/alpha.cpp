#include "beta.cpp"

struct AlphaBase {
  virtual ~AlphaBase() = default;
  virtual void tick() = 0;
};

struct Alpha final : AlphaBase {
  Beta *member = nullptr;

  Beta *make(Beta *value) { return value; }

  void tick() override {}
};

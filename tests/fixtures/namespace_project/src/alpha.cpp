namespace a::c {
struct Leaf;
}

namespace a::b {

struct Interface {
  virtual ~Interface() = default;
  virtual void run() = 0;
};

struct Concrete final : Interface {
  a::c::Leaf *dependency = nullptr;

  void run() override {}
};

} // namespace a::b

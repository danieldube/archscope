#include <catch2/catch_test_macros.hpp>

#include "clang/parallel_executor.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>

TEST_CASE("parallel executor uses multiple worker threads when requested",
          "[clang][parallel]") {
  std::atomic<unsigned> ready_workers{0U};
  std::atomic<bool> overlap_observed{false};
  std::mutex thread_ids_mutex;
  std::set<std::thread::id> thread_ids;

  archscope::clang_backend::run_in_parallel(2U, 2U, [&](const std::size_t) {
    {
      std::lock_guard<std::mutex> lock(thread_ids_mutex);
      thread_ids.insert(std::this_thread::get_id());
    }

    ready_workers.fetch_add(1U);
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
      if (ready_workers.load() >= 2U) {
        overlap_observed.store(true);
        break;
      }
      std::this_thread::yield();
    }
  });

  REQUIRE(overlap_observed.load());
  REQUIRE(thread_ids.size() >= 2U);
}

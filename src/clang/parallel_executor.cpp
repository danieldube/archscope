#include "clang/parallel_executor.hpp"

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

namespace archscope::clang_backend {

void run_in_parallel(const std::size_t item_count, const unsigned thread_count,
                     const std::function<void(std::size_t)> &job) {
  if (item_count == 0U) {
    return;
  }

  const unsigned worker_count =
      std::max(1U, std::min(thread_count, static_cast<unsigned>(item_count)));
  std::atomic<std::size_t> next_index{0U};
  std::vector<std::thread> workers;
  workers.reserve(worker_count);

  for (unsigned worker_index = 0; worker_index < worker_count; ++worker_index) {
    workers.emplace_back([&job, &next_index, item_count]() {
      while (true) {
        const std::size_t index = next_index.fetch_add(1U);
        if (index >= item_count) {
          return;
        }

        job(index);
      }
    });
  }

  for (std::thread &worker : workers) {
    worker.join();
  }
}

} // namespace archscope::clang_backend

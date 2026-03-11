#pragma once

#include <cstddef>
#include <functional>

namespace archscope::clang_backend {

void run_in_parallel(std::size_t item_count, unsigned thread_count,
                     const std::function<void(std::size_t)> &job);

} // namespace archscope::clang_backend

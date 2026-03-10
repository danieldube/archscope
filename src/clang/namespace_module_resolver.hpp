#pragma once

#include <string>

namespace clang {
class Decl;
}

namespace archscope::clang_backend {

std::string resolve_namespace_module(const clang::Decl &declaration);

} // namespace archscope::clang_backend

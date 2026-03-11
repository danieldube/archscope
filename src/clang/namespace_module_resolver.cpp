#include "clang/namespace_module_resolver.hpp"

#include <clang/AST/Decl.h>

#include <algorithm>
#include <string>
#include <vector>

namespace archscope::clang_backend {

namespace {

using clang::DeclContext;
using clang::NamespaceDecl;

std::string namespace_component(const NamespaceDecl &declaration) {
  if (declaration.isAnonymousNamespace()) {
    return "<anonymous>";
  }

  return declaration.getNameAsString();
}

} // namespace

std::string resolve_namespace_module(const clang::Decl &declaration) {
  std::vector<std::string> components;

  const DeclContext *context = declaration.getDeclContext();
  while (context != nullptr) {
    if (const auto *namespace_decl =
            llvm::dyn_cast<NamespaceDecl>(context)) {
      components.push_back(namespace_component(*namespace_decl));
    }
    context = context->getParent();
  }

  if (components.empty()) {
    return "<global>";
  }

  std::reverse(components.begin(), components.end());

  std::string module_name;
  for (std::size_t index = 0; index < components.size(); ++index) {
    if (index > 0U) {
      module_name += "::";
    }
    module_name += components[index];
  }

  return module_name;
}

} // namespace archscope::clang_backend

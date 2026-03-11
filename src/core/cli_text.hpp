#pragma once

#include <string>
#include <utility>
#include <vector>

namespace archscope::core {

using CliDetail = std::pair<std::string, std::string>;

std::string HelpText();
std::string FormatErrorText(const std::string &category,
                            const std::string &message,
                            const std::vector<CliDetail> &details = {});
std::string FormatInfoText(const std::string &message);

} // namespace archscope::core

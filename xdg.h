#pragma once

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace xdg {

fs::path runtime_dir();

} // namespace xdg

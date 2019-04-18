#pragma once

#include <filesystem>

namespace xdg {

std::filesystem::path config_home();
std::filesystem::path find_config(std::filesystem::path relative);

std::filesystem::path data_home();
std::filesystem::path find_data(std::filesystem::path relative);

} // namespace xdg

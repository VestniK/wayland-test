#pragma once

#include <chrono>
#include <filesystem>
#include <system_error>

struct script_window {
  virtual ~script_window() = default;

  virtual std::error_code draw_frame() = 0;
  virtual std::error_code draw_for(std::chrono::milliseconds) = 0;
};

void play_script(const std::filesystem::path& path, script_window& wnd);
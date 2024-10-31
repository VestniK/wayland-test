#pragma once

#include <filesystem>
#include <unordered_map>

#include <thinsys/io/io.hpp>

namespace sfx {

class archive {
public:
  struct entry {
    size_t size = 0;
    size_t offset = 0;
    bool offset_is_adjusted = false;
  };

  const std::unordered_map<fs::path, entry>& entries() const noexcept {
    return entries_;
  }

  thinsys::io::file_descriptor& open(entry& e);
  thinsys::io::file_descriptor& open(const fs::path& path);

  static archive open_self();

private:
  archive(std::unordered_map<fs::path, entry> entries,
      thinsys::io::file_descriptor fd)
      : entries_{std::move(entries)}, fd_{std::move(fd)} {}

private:
  std::unordered_map<fs::path, entry> entries_;
  thinsys::io::file_descriptor fd_;
};

} // namespace sfx

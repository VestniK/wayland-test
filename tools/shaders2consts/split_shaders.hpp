#pragma once

#include <concepts>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tools/shaders2consts/parse_include.hpp>

template <typename T>
concept resolver = requires(T& r, const std::filesystem::path& relative) {
  { r(relative) } -> std::derived_from<std::istream>;
};

class shader_pieces_builder {
public:
  std::vector<std::string> finish() && { return std::move(pieces_); }

  template <resolver R>
  std::vector<size_t> parse_shader(std::istream& in, R& resolver) {
    std::vector<size_t> res;

    using enum include_parse_error;
    std::stringstream current;

    bool last_line_nonempty = false;
    auto finish_piece = [&] {
      if (current.str().empty())
        return;
      res.push_back(pieces_.size());
      pieces_.push_back(std::exchange(current, {}).str());
      last_line_nonempty = false;
    };

    std::string ln;
    while (std::getline(in, ln)) {
      const auto parse_res = parse_include(ln);
      if (parse_res.has_value()) {
        finish_piece();
        auto [it, inserted] = once_handled_includes_.emplace(
            parse_res.value(), std::vector<size_t>{});
        if (inserted) {
          auto include = resolver(it->first);
          it->second = parse_shader(include, resolver);
        }
        std::ranges::copy(it->second, std::back_inserter(res));
        last_line_nonempty = false;
        continue;
      }

      switch (parse_res.error()) {
      case non_include_line:
        if (!ln.empty() || last_line_nonempty)
          current << ln << '\n';
        last_line_nonempty = !ln.empty();
        break;
      case include_parse_error::missing_close_quot:
      case include_parse_error::missing_open_quot:
        // TODO;
        break;
      }
    }
    finish_piece();
    return res;
  }

private:
  std::vector<std::string> pieces_;
  std::unordered_map<std::filesystem::path, std::vector<size_t>>
      once_handled_includes_;
};

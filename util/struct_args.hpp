#pragma once

#include <optional>
#include <ostream>
#include <span>
#include <string_view>
#include <vector>

#include <util/get_option.hpp>

namespace args {

namespace detail {

struct option_info {
  std::string_view long_name;
  std::string_view short_name;
  std::string_view description;
};

struct parser_iface {
  virtual const char* get_option(const option_info& opt) = 0;
  virtual const char* get_required_option(const option_info& opt) = 0;
};

parser_iface* current_parser = nullptr;

class arguments_parser : public parser_iface {
public:
  arguments_parser(std::span<char*> args) : args_{args} {}

  const char* get_option(const option_info& opt) override {
    const char* val = ::get_option(args_, opt.long_name);
    if (!val)
      val = ::get_option(args_, opt.short_name);
    return val;
  }

  const char* get_required_option(const option_info& opt) override {
    const char* val = ::get_option(args_, opt.long_name);
    if (!val)
      val = ::get_option(args_, opt.short_name);
    if (!val)
      missing_opts_.push_back(opt.long_name);
    return val;
  }

private:
  std::span<char*> args_;
  std::vector<std::string_view> missing_opts_;
};

class args_help_parser : public parser_iface {
public:
  args_help_parser(std::ostream& out) noexcept : out_{out} {}

  const char* get_option(const option_info& opt) override {
    out_ << '\t' << opt.long_name << (opt.short_name.empty() ? "" : ", ")
         << opt.short_name << " VAL\t" << opt.description << '\n';
    return nullptr;
  }

  const char* get_required_option(const option_info& opt) override {
    return get_option(opt);
  }

private:
  std::ostream& out_;
};

class usage_parser : public parser_iface {
public:
  usage_parser(std::ostream& out) noexcept : out_{out} {}

  const char* get_option(const option_info& opt) override {
    out_ << " [" << opt.long_name << " VAL]";
    return nullptr;
  }
  const char* get_required_option(const option_info& opt) override {
    out_ << ' ' << (opt.short_name.empty() ? opt.long_name : opt.short_name)
         << " VAL";
    return nullptr;
  }

private:
  std::ostream& out_;
};

} // namespace detail

template <typename T>
class option : private detail::option_info {
public:
  option(const char* long_name, const char* description)
      : detail::option_info{.long_name = long_name,
            .short_name = {},
            .description = description} {}

  option(const char* short_name, const char* long_name, const char* description)
      : detail::option_info{.long_name = long_name,
            .short_name = short_name,
            .description = description} {}

  option& default_value(const T& val) {
    default_ = val;
    return *this;
  }

  operator T() const {
    const char* val = default_
                          ? detail::current_parser->get_option(*this)
                          : detail::current_parser->get_required_option(*this);
    return val ? T{val} : default_.value_or(T{});
  }

private:
  std::optional<T> default_;
};

template <typename T>
T parse(std::span<char*> args) {
  detail::arguments_parser p{args};
  detail::current_parser = &p;
  return T{};
}

template <typename T>
void args_help(std::ostream& out) {
  detail::args_help_parser p{out};
  detail::current_parser = &p;
  T{};
}

template <typename T>
void usage(std::string_view progname, std::ostream& out) {
  out << "Usage: " << progname;
  detail::usage_parser p{out};
  detail::current_parser = &p;
  T{};
  out << '\n';
}

} // namespace args

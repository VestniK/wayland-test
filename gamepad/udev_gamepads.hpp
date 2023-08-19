#pragma once

#include <cstdint>
#include <filesystem>
#include <iterator>
#include <memory>
#include <unordered_map>

#include <libudev.h>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <gamepad/evdev_gamepad.hpp>

namespace detail::udev {

struct deleter {
  void operator()(::udev* p) const noexcept { udev_unref(p); }
  void operator()(udev_enumerate* p) const noexcept { udev_enumerate_unref(p); }
  void operator()(udev_device* p) const noexcept { udev_device_unref(p); }
  void operator()(udev_monitor* p) const noexcept { udev_monitor_unref(p); }
};

template <typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

class list_iterator {
public:
  using difference_type = std::ptrdiff_t;
  using value_type = udev_list_entry;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::forward_iterator_tag;

  list_iterator() noexcept = default;
  list_iterator(udev_list_entry* head) noexcept : head_{head} {}

  bool operator==(const list_iterator&) const noexcept = default;

  reference operator*() const { return *head_; }
  list_iterator& operator++() {
    head_ = udev_list_entry_get_next(head_);
    return *this;
  }

  list_iterator operator++(int) {
    list_iterator last = *this;
    head_ = udev_list_entry_get_next(head_);
    return last;
  }

private:
  udev_list_entry* head_ = nullptr;
};

struct list_range {
  udev_list_entry* head;
};
inline list_iterator begin(list_range& list) noexcept { return {list.head}; }
inline list_iterator end(list_range&) noexcept { return {}; }

} // namespace detail::udev

class udev_gamepads {
public:
  class connected_gamepads {
  public:
    using iterator = detail::udev::list_iterator;

    connected_gamepads(udev& udev);

    iterator begin() noexcept {
      return {udev_enumerate_get_list_entry(enumerator_.get())};
    }
    iterator end() const noexcept { return {}; };

  private:
    detail::udev::unique_ptr<udev_enumerate> enumerator_;
  };

  udev_gamepads(evdev_gamepad::key_handler key_handler,
      evdev_gamepad::axis_state& axis_state);

  connected_gamepads connected() { return {*udev_}; }
  asio::awaitable<void> watch(asio::io_context::executor_type exec);
  udev& native_handle() const noexcept { return *udev_; }

private:
  void on_add(asio::io_context::executor_type exec, std::string_view devnode,
      udev_device& dev);
  void on_remove(std::string_view devnode, udev_device& dev);

private:
  detail::udev::unique_ptr<udev> udev_{udev_new()};
  detail::udev::unique_ptr<udev_monitor> monitor_{
      udev_monitor_new_from_netlink(udev_.get(), "udev")};
  std::unordered_map<std::filesystem::path, evdev_gamepad> gamepads_;
  evdev_gamepad::key_handler key_handler_;
  evdev_gamepad::axis_state* axis_state_ = nullptr;
};

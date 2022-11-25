#pragma once

#include <concepts>
#include <system_error>

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <wayland/wayland-client.hpp>

template <typename Service> struct identified {
  wl::unique_ptr<Service> service;
  uint32_t id = {};
};

class event_loop;

class queues_notify_callback {
public:
  queues_notify_callback(event_loop &eloop) noexcept : eloop_{eloop} {}

  void operator()() noexcept;

private:
  std::reference_wrapper<event_loop> eloop_;
};

class event_queue {
public:
  event_queue(event_loop &eloop) noexcept;

  wl_event_queue &get() const noexcept { return *queue_; }
  void dispatch();
  void dispatch_pending();
  wl_display &display() const noexcept;

  queues_notify_callback notify_callback() noexcept { return {eloop_.get()}; }

private:
  std::reference_wrapper<event_loop> eloop_;
  wl::unique_ptr<wl_event_queue> queue_;
  unsigned last_read_processed_ = 0;
};

class event_loop {
public:
  event_loop(const char *display);

  [[nodiscard]] wl_display &get_display() const noexcept { return *display_; }
  [[nodiscard]] wl_compositor *get_compositor() const noexcept {
    return compositor_.service.get();
  }
  [[nodiscard]] ivi_application *get_ivi() const noexcept {
    return ivi_.service.get();
  }
  [[nodiscard]] xdg_wm_base *get_xdg_wm() const noexcept {
    return xdg_wm_.service.get();
  }

  void dispatch(std::error_code &ec) noexcept;
  void dispatch();
  void dispatch_pending(std::error_code &ec) noexcept;
  void dispatch_pending();

  event_queue make_queue() noexcept { return event_queue{*this}; }

  asio::awaitable<void> dispatch_once(asio::io_context::executor_type exec);
  template <std::predicate Pred>
  asio::awaitable<void> dispatch_while(asio::io_context::executor_type exec,
                                       Pred &&pred) {
    while (pred())
      co_await dispatch_once(exec);
    co_return;
  }

  void notify_queues() noexcept {
    ++read_count_;
    read_count_.notify_all();
  }

private:
  friend class event_queue;

  std::error_code check() noexcept;

  static void global(void *data, wl_registry *reg, uint32_t id,
                     const char *name, uint32_t ver);
  static void global_remove(void *data, wl_registry *, uint32_t id);

private:
  wl::unique_ptr<wl_display> display_;
  std::atomic<unsigned> read_count_ = 0;
  wl::unique_ptr<wl_registry> registry_;
  wl_registry_listener listener_ = {&global, &global_remove};
  identified<wl_compositor> compositor_;
  identified<ivi_application> ivi_;
  identified<xdg_wm_base> xdg_wm_;
};

inline event_queue::event_queue(event_loop &eloop) noexcept
    : eloop_{eloop}, queue_{wl_display_create_queue(&eloop.get_display())} {}

inline void event_queue::dispatch() {
  eloop_.get().read_count_.wait(last_read_processed_);
  dispatch_pending();
}

inline void event_queue::dispatch_pending() {
  last_read_processed_ = eloop_.get().read_count_.load();
  wl_display_dispatch_queue_pending(&display(), queue_.get());
}

inline wl_display &event_queue::display() const noexcept {
  return eloop_.get().get_display();
}

inline void queues_notify_callback::operator()() noexcept {
  eloop_.get().notify_queues();
}

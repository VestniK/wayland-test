#pragma once

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <libs/sync/heartbeat.hpp>

#include <libs/wlwnd/wlutil.hpp>

template <typename Service>
struct identified {
  wl::unique_ptr<Service> service;
  uint32_t id = {};
};

class event_loop;

class event_queue {
public:
  event_queue() noexcept = default;
  event_queue(event_loop& eloop) noexcept;

  wl_event_queue& get() const noexcept { return *queue_; }
  void dispatch();
  void dispatch_pending();
  wl_display& display() const noexcept { return *display_; }

  void wake() const noexcept { waiter_.wake(); }

private:
  heartbeat_waiter waiter_;
  wl_display* display_ = nullptr;
  wl::unique_ptr<wl_event_queue> queue_;
};

class event_loop {
public:
  event_loop(const char* display);

  [[nodiscard]] wl_display& get_display() const noexcept { return *display_; }

  void dispatch() noexcept;
  void dispatch_pending() noexcept;

  event_queue make_queue() noexcept { return event_queue{*this}; }

  asio::awaitable<void> dispatch_once(asio::io_context::executor_type exec);
  template <std::predicate Pred>
  asio::awaitable<void> dispatch_while(asio::io_context::executor_type exec, Pred&& pred) {
    while (pred())
      co_await dispatch_once(exec);
    co_return;
  }

private:
  friend class event_queue;

private:
  wl::unique_ptr<wl_display> display_;
  heartbeat heartbeat_;
};

inline event_queue::event_queue(event_loop& eloop) noexcept
    : waiter_{eloop.heartbeat_.make_waiter()}, display_{eloop.display_.get()},
      queue_{wl_display_create_queue(&eloop.get_display())} {}

inline void event_queue::dispatch() {
  if (waiter_.wait_new_beats() != 0)
    dispatch_pending();
}

inline void event_queue::dispatch_pending() { wl_display_dispatch_queue_pending(display_, queue_.get()); }

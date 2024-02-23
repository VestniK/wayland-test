#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

#include <util/geom.hpp>

struct wl_surface;
namespace xdg {
struct delegate;
}

namespace wl {

template <typename T>
concept window = requires(T& t, xdg::delegate& d) {
  { t.get_surface() } -> std::same_as<wl_surface&>;
  { t.set_delegate(&d) };
} && std::movable<T>;

template <window Wnd>
struct sized_window {
  Wnd window;
  size sz;
};

class shell_window {
private:
  static constexpr std::size_t align = alignof(void*);
  static constexpr std::size_t storage = 4 * sizeof(void*);

  struct iface {
    virtual ~iface() noexcept = default;
    virtual iface* move_to(std::byte (&)[storage]) noexcept = 0;
    virtual wl_surface& get_surface() = 0;
    virtual void set_delegate(xdg::delegate* delegate) = 0;
  };

public:
  shell_window() noexcept = default;

  shell_window(const shell_window&) = delete;
  shell_window& operator=(const shell_window&) = delete;

  shell_window(shell_window&& rhs) noexcept {
    if (rhs.is_embedded())
      iface_ = std::exchange(rhs.iface_, nullptr)->move_to(storage_);
    else
      iface_ = std::exchange(rhs.iface_, nullptr);
  }
  shell_window& operator=(shell_window&& rhs) noexcept {
    destroy();
    if (rhs.is_embedded())
      iface_ = std::exchange(rhs.iface_, nullptr)->move_to(storage_);
    else
      iface_ = std::exchange(rhs.iface_, nullptr);
    return *this;
  }

  ~shell_window() noexcept { destroy(); }

  template <window Wnd>
  shell_window(Wnd&& wnd) noexcept {
    struct box : Wnd, iface {
      box(Wnd&& rhs) noexcept : Wnd{std::move(rhs)} {}
      wl_surface& get_surface() override { return Wnd::get_surface(); }
      void set_delegate(xdg::delegate* delegate) override {
        return Wnd::set_delegate(delegate);
      }
      iface* move_to(std::byte (&dest)[storage]) noexcept override {
        static_assert(sizeof(box) <= storage && alignof(box) <= align);
        iface* res = new (dest) box{static_cast<Wnd&&>(*this)};
        this->~iface();
        return res;
      }
    };
    if constexpr (sizeof(box) <= storage && alignof(box) <= align) {
      iface_ = new (storage_) box{std::move(wnd)};
    } else {
      iface_ = new box{std::move(wnd)};
    }
  }

  wl_surface& get_surface() { return iface_->get_surface(); }
  void set_delegate(xdg::delegate* delegate) { iface_->set_delegate(delegate); }

  explicit operator bool() const noexcept { return !iface_; }

private:
  bool is_embedded() const noexcept {
    return reinterpret_cast<uintptr_t>(iface_) ==
           reinterpret_cast<uintptr_t>(&storage_);
  }
  void destroy() noexcept {
    if (is_embedded()) {
      iface_->~iface();
    } else
      delete iface_;
  }

private:
  alignas(align) std::byte storage_[storage];
  iface* iface_ = nullptr;
};

} // namespace wl

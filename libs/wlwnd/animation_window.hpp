#include <libs/corort/executors.hpp>
#include <libs/sync/channel.hpp>

#include <libs/wlwnd/event_loop.hpp>
#include <libs/wlwnd/renderer.hpp>
#include <libs/wlwnd/shell_window.hpp>

namespace wl {
class gui_shell;
}

class animation_window {
public:
  animation_window() noexcept = default;
  animation_window(
      event_queue queue, co::pool_executor pool_exec, wl::sized_window<wl::shell_window>&& wnd,
      animation_function animation_func
  );

  animation_window(const animation_window&) = delete;
  animation_window& operator=(const animation_window&) = delete;

  animation_window(animation_window&&) noexcept;
  animation_window& operator=(animation_window&&) noexcept;

  ~animation_window() noexcept;

  [[nodiscard]] bool is_closed() const noexcept;

private:
  struct impl;

private:
  wl::shell_window wnd_;
  event_queue queue_;
  std::unique_ptr<impl> impl_;
};

#pragma once

#include <linux/input-event-codes.h>

#include <chrono>

#include "basic_resource.hpp"
#include "error.hpp"
#include "surface.hpp"
#include "util.hpp"

namespace wl {

namespace detail {

enum class button: uint32_t {
  misc =  BTN_MISC,
  misc0 =  BTN_0,
  misc1 =  BTN_1,
  misc2 =  BTN_2,
  misc3 =  BTN_3,
  misc4 =  BTN_4,
  misc5 =  BTN_5,
  misc6 =  BTN_6,
  misc7 =  BTN_7,
  misc8 =  BTN_8,
  misc9 =  BTN_9,

  mouse =  BTN_MOUSE,
  left = BTN_LEFT,
  right =  BTN_RIGHT,
  middle =  BTN_MIDDLE,
  side =  BTN_SIDE,
  extra =  BTN_EXTRA,
  forward =  BTN_FORWARD,
  back =  BTN_BACK,
  task =  BTN_TASK,

  joystick =  BTN_JOYSTICK,
  trigger =  BTN_TRIGGER,
  thumb =  BTN_THUMB,
  thumb2 =  BTN_THUMB2,
  top =  BTN_TOP,
  top2 =  BTN_TOP2,
  pinkie =  BTN_PINKIE,
  base =  BTN_BASE,
  base2 =  BTN_BASE2,
  base3 =  BTN_BASE3,
  base4 =  BTN_BASE4,
  base5 =  BTN_BASE5,
  base6 =  BTN_BASE6,
  dead =  BTN_DEAD,

  gaempad =  BTN_GAMEPAD,
  south =  BTN_SOUTH,
  a =  BTN_A,
  east =  BTN_EAST,
  b =  BTN_B,
  c =  BTN_C,
  north =  BTN_NORTH,
  x =  BTN_X,
  west =  BTN_WEST,
  y =  BTN_Y,
  z =  BTN_Z,
  tl =  BTN_TL,
  tr =  BTN_TR,
  tl2 =  BTN_TL2,
  tr2 =  BTN_TR2,
  select =  BTN_SELECT,
  start =  BTN_START,
  mode =  BTN_MODE,
  thumbl =  BTN_THUMBL,
  thumbr =  BTN_THUMBR,

  digi =  BTN_DIGI,
  tool_pen =  BTN_TOOL_PEN,
  tool_rubber =  BTN_TOOL_RUBBER,
  tool_brush =  BTN_TOOL_BRUSH,
  tool_pencil =  BTN_TOOL_PENCIL,
  tool_airbrush =  BTN_TOOL_AIRBRUSH,
  tool_finger =  BTN_TOOL_FINGER,
  tool_mouse =  BTN_TOOL_MOUSE,
  tool_lens =  BTN_TOOL_LENS,
  tool_quinttap =  BTN_TOOL_QUINTTAP,
  stylus3 =  BTN_STYLUS3,
  touch =  BTN_TOUCH,
  stylus =  BTN_STYLUS,
  stylus2 =  BTN_STYLUS2,
  tool_doubletap =  BTN_TOOL_DOUBLETAP,
  tool_tripletap =  BTN_TOOL_TRIPLETAP,
  tool_quadtap =  BTN_TOOL_QUADTAP,

  wheel =  BTN_WHEEL,
  gear_down =  BTN_GEAR_DOWN,
  gear_up =  BTN_GEAR_UP
};

enum class button_state {
  released = WL_POINTER_BUTTON_STATE_RELEASED,
  pressed = WL_POINTER_BUTTON_STATE_PRESSED
};

enum class axis {
  vertical_scroll = WL_POINTER_AXIS_VERTICAL_SCROLL,
  horizontal_scroll = WL_POINTER_AXIS_HORIZONTAL_SCROLL
};

enum class axis_source {
  wheel = WL_POINTER_AXIS_SOURCE_WHEEL,
  finger = WL_POINTER_AXIS_SOURCE_FINGER ,
  continous = WL_POINTER_AXIS_SOURCE_CONTINUOUS,
  wheel_tilt = WL_POINTER_AXIS_SOURCE_WHEEL_TILT
};

template<typename PT>
struct pointer {
  using button = wl::detail::button;
  using button_state = wl::detail::button_state;
  using axis = wl::detail::axis;
  using axis_source = wl::detail::axis_source;

  template<typename F>
  void add_listener(F& listener) {
    static const wl_pointer_listener static_listener= {
      [](void* data, wl_pointer* handle, uint32_t serial, wl_surface* surf, wl_fixed_t x, wl_fixed_t y) {
        auto self = static_cast<F*>(data);
        self->enter(resource_ref_t<PT>{*handle}, wl::serial{serial}, wl::surface::ref{*surf}, fixed_point{x, y});
      },
      [](void* data, wl_pointer* handle, uint32_t serial, wl_surface* surf) {
        auto self = static_cast<F*>(data);
        self->leave(resource_ref_t<PT>{*handle}, wl::serial{serial}, wl::surface::ref{*surf});
      },
      [](void* data, wl_pointer* handle, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
        auto self = static_cast<F*>(data);
        self->motion(resource_ref_t<PT>{*handle}, wl::clock::time_point{wl::clock::duration{time}}, fixed_point{x, y});
      },
      [](void* data,   struct wl_pointer* handle, uint32_t eid, uint32_t time, uint32_t btn, uint32_t state) {
        auto self = static_cast<F*>(data);
        self->button(
          resource_ref_t<PT>{*handle}, serial{eid}, clock::time_point{clock::duration{time}},
          button{btn}, button_state{state}
        );
      },
      [](void* data, wl_pointer* handle, uint32_t time, uint32_t axis_val, wl_fixed_t value) {
        auto self = static_cast<F*>(data);
        self->axis(resource_ref_t<PT>{*handle}, clock::time_point{clock::duration{time}}, axis{axis_val}, value);
      },
      [](void* data, wl_pointer* handle) {
        auto self = static_cast<F*>(data);
        self->frame(resource_ref_t<PT>{*handle});
      },
      [](void* data, wl_pointer* handle, uint32_t source) {
        auto self = static_cast<F*>(data);
        self->axis_source(resource_ref_t<PT>{*handle}, axis_source{source});
      },
      [](void* data, wl_pointer* handle, uint32_t time, uint32_t axis_id) {
        auto self = static_cast<F*>(data);
        self->axis_stop(resource_ref_t<PT>{*handle}, clock::time_point{clock::duration{time}}, axis{axis_id});
      },
      [](void* data, wl_pointer* handle, uint32_t axis_id, int32_t discrete) {
        auto self = static_cast<F*>(data);
        self->axis_discrete(resource_ref_t<PT>{*handle}, axis{axis_id}, discrete);
      }
    };
    if (wl_pointer_add_listener(native_handle<PT>(*this), &static_listener, &listener) != 0)
      throw std::system_error{errc::add_keyboard_listener_failed};
  }
};

}

using pointer = detail::basic_resource<wl_pointer, detail::pointer>;

}

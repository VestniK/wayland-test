#include <chrono>
#include <filesystem>
#include <iostream>

#include <chaiscript/chaiscript.hpp>

#include <wayland/script_player.hpp>
#include <wayland/util/xdg.hpp>

using namespace std::literals;
namespace fs = std::filesystem;

namespace {

// chaiscript конвертирует исключения между плюсами и скриптом, позволяя в
// скрипте ловить любые плюсовые исключения отнаследованные от std::exception.
// Исключение для принудительной остановки скрипта нельзя довать перехватывать
// внутри него, поэтому заводим специальный тип маркер для этого.
//
// TODO: Избавиться от этого костыля когда в chaiscript запилят эту фичу:
// https://github.com/ChaiScript/ChaiScript/issues/452
enum class stop_script {};

template <typename Unit>
void add_unit_class(chaiscript::Module& module, const std::string& name) {
  chaiscript::utility::add_class<Unit>(module, name,
      {chaiscript::constructor<Unit()>(),
          chaiscript::constructor<Unit(const Unit&)>()},
      {
          {chaiscript::fun([](Unit l, Unit r) { return l + r; }), "+"},
          {chaiscript::fun([](Unit l, Unit r) { return l - r; }), "-"},
          {chaiscript::fun(
               [](typename Unit::rep num, Unit unit) { return num * unit; }),
              "*"},
          {chaiscript::fun(
               [](Unit unit, typename Unit::rep num) { return unit * num; }),
              "*"},
          {chaiscript::fun(
               [](Unit unit, typename Unit::rep num) { return unit / num; }),
              "/"},
          {chaiscript::fun([](Unit l, Unit r) { return l / r; }), "/"},
      });
  if constexpr (std::is_integral_v<typename Unit::rep>) {
    module.add(chaiscript::fun([](Unit unit, typename Unit::rep num) {
      return unit % num;
    }),
        "%");
    module.add(chaiscript::fun([](Unit l, Unit r) { return l % r; }), "%");
  }
}

chaiscript::ModulePtr create_chrono_module() {
  auto res = std::make_shared<chaiscript::Module>();
  add_unit_class<std::chrono::milliseconds>(*res, "milliseconds");
  res->add_global_const(chaiscript::const_var(1ms), "ms");
  res->add_global_const(
      chaiscript::const_var(
          std::chrono::duration_cast<std::chrono::milliseconds>(1s)),
      "s");
  res->add_global_const(
      chaiscript::const_var(
          std::chrono::duration_cast<std::chrono::milliseconds>(1min)),
      "min");
  res->add_global_const(
      chaiscript::const_var(
          std::chrono::duration_cast<std::chrono::milliseconds>(1h)),
      "h");
  return res;
}

chaiscript::ModulePtr create_xdg_module() {
  auto res = std::make_shared<chaiscript::Module>();
  res->add(chaiscript::type_conversion<std::string, std::filesystem::path>());
  res->add(chaiscript::fun(xdg::find_data), "xdg_find_data");
  return res;
}

chaiscript::ModulePtr create_wnd_module() {
  auto res = std::make_shared<chaiscript::Module>();
  res->add(chaiscript::fun([](script_window& wnd) {
    if (auto ec = wnd.draw_frame()) {
      std::clog << "Script stops due to error: [" << ec << "] " << ec.message()
                << '\n';
      throw stop_script{};
    }
  }),
      "draw_frame");
  res->add(chaiscript::fun(
               [](script_window& wnd, std::chrono::milliseconds duration) {
                 if (auto ec = wnd.draw_for(duration)) {
                   std::clog << "Script stops due to error: [" << ec << "] "
                             << ec.message() << '\n';
                   throw stop_script{};
                 }
               }),
      "draw_for");
  return res;
}

} // namespace

void play_script(const std::filesystem::path& path, script_window& wnd) {
  chaiscript::ChaiScript engine;
  engine.add(chaiscript::var(std::ref(wnd)), "window");
  engine.add(create_wnd_module());
  engine.add(create_chrono_module());
  engine.add(create_xdg_module());
  try {
    engine.eval_file(path.string());
  } catch (stop_script) {
  }
}

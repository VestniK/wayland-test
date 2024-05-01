find_package(OpenGL REQUIRED COMPONENTS EGL)
find_package(Catch2 REQUIRED)
find_package(asio REQUIRED)
find_package(spdlog REQUIRED)

cpp_unit(
  NAME wlwnd
  STD cxx_std_23
  LIBS
    sync
    util
    vk
    Wayland::client Wayland::egl
    OpenGL::EGL
    asio::asio
    spdlog::spdlog
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
target_wl_protocol(wlwnd
  NAME xdg-shell
  URL https://gitlab.freedesktop.org/wayland/wayland-protocols/-/raw/4624cfaaf563cd7be5e2e2087c8de6d3a48ea867/stable/xdg-shell/xdg-shell.xml
  SHA256 5b63a655af7147381705c32744fde4ac3f54ee6147396f709f394b0d1f711e71
)
target_wl_protocol(wlwnd
  NAME ivi-application
  URL https://github.com/COVESA/wayland-ivi-extension/raw/2.2.0/protocol/ivi-application.xml
  SHA256 fbbdee032617675eae78d9bda864af519a3a34479d188740d938d17ffacf80d2
)

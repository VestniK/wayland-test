find_package(Catch2 REQUIRED)
find_package(asio REQUIRED)
find_package(spdlog REQUIRED)

cpp_unit(
  NAME wlwnd
  STD cxx_std_23
  LIBS
    anime
    sync
    geom
    Wayland::client
    asio::asio
    spdlog::spdlog
  TEST_LIBS
    Catch2::Catch2
    Catch2::Catch2WithMain
  TEST_ARGS --order rand --rng-seed time
)
target_wl_protocol(wlwnd
  NAME xdg-shell
  URL https://gitlab.freedesktop.org/wayland/wayland-protocols/-/raw/1.36/stable/xdg-shell/xdg-shell.xml?ref_type=tags
  SHA256 454c96a942bfd7b21acdceb74d189cee85858afb7e7d2274964c94f13616f69f
)
target_wl_protocol(wlwnd
  NAME ivi-application
  URL https://raw.githubusercontent.com/COVESA/wayland-ivi-extension/2.3.2/protocol/ivi-application.xml
  SHA256 34affef72f270d9bac7f4f4ff9176be157940923dc7f0d7622fc66683bc116dd
)

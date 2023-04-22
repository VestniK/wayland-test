cxx_binary(
  name = "wayland-test",
  srcs = ["wayland/main.cpp"],
  include_directories = ["/"],
  deps = [
    "//.buck/conan:spdlog",
    "//.buck/conan:asio",
    "//util:util",
    "//corort:corort",
    "//gles:gles",
  ],
)

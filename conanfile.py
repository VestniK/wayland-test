from conan import ConanFile


class Deps(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("asio/1.36.0")
        self.requires("fmt/12.1.0", override=True)
        self.requires("glm/cci.20230113")
        self.requires("libpng/1.6.44")
        self.requires("mp-units/2.4.0", options={'std_format': False})
        self.requires("spdlog/1.16.0")
        self.requires("tracy/cci.20220130")
        self.requires("vulkan-headers/1.3.296.0", override=True)
        self.requires("vulkan-memory-allocator/3.0.1")
        self.requires("freetype/2.13.3")

    def build_requirements(self):
        self.test_requires("catch2/3.7.1")

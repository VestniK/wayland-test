from conan import ConanFile


class Deps(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("asio/1.31.0")
        self.requires("fmt/11.0.1", override=True)
        self.requires("glm/cci.20230113")
        self.requires("libpng/1.6.44")
        self.requires("mp-units/2.3.0")
        self.requires("spdlog/1.14.1")
        self.requires("tracy/cci.20220130")
        self.requires("vulkan-headers/1.3.290.0")

    def build_requirements(self):
        self.test_requires("catch2/3.7.1")

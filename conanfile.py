from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout


class UltrasoundProcessorConan(ConanFile):
    name = "ultrasound-processor"
    version = "0.1.0"
    license = "MIT"
    description = "Standalone ultrasound processor with ImGui visualization"
    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "with_tests": [True, False],
        "with_visualizer": [True, False],
    }
    default_options = {
        "with_tests": True,
        "with_visualizer": True,
        "imgui/*:glfw": True,
        "imgui/*:opengl3": True,
    }

    def configure(self):
        if self.options.with_visualizer:
            self.options["imgui/*"].glfw = True
            self.options["imgui/*"].opengl3 = True

    def requirements(self):
        if self.options.with_tests:
            self.requires("gtest/1.14.0")
        if self.options.with_visualizer:
            self.requires("glfw/3.4")
            self.requires("glew/2.2.0")
            self.requires("imgui/cci.20230105+1.89.2.docking")
            self.requires("opengl/system")

    def build_requirements(self):
        self.tool_requires("cmake/3.30.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        tc.cache_variables["ULTRASOUND_BUILD_TESTS"] = bool(self.options.with_tests)
        tc.cache_variables["ULTRASOUND_WITH_VISUALIZER"] = bool(self.options.with_visualizer)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.with_tests:
            cmake.test()

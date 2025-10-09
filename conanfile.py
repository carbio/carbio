import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, load, rmdir

class CarbioRecipe(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    
    def configure(self):
        self.options["spdlog"].no_exceptions = True
        self.options["spdlog"].header_only = True
        self.options["spdlog"].use_std_fmt = False

    def requirements(self):
        # Project dependencies
        self.requires("spdlog/1.15.3")
        # Test dependencies
        self.test_requires("gtest/1.16.0")
        self.test_requires("benchmark/1.9.4")

    def layout(self):
        cmake_layout(self)
        self.folders.generators = f"{self.recipe_folder}/build/generators"

    def _generate_cmake_toolchain(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = None
        tc.generate()

    def _generate_cmake_dependencies(self):
        deps = CMakeDeps(self)
        deps.generate()

    def generate(self):
        self._generate_cmake_toolchain()
        self._generate_cmake_dependencies()

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
        cmake.ctest(cli_args=["--output-on-failure"])

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
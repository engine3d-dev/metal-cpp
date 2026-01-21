from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout, CMakeToolchain, CMakeDeps
from conan.tools.files import copy
from pathlib import Path


required_conan_version = ">=2.2.0"


class LibraryRecipe(ConanFile):
    name = "metalcpp"
    version = "1.0"
    license = "Apache-2.0"
    url = "https://github.com/engine3d-dev/metal-cpp"
    settings = "compiler", "build_type", "os", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    exports_sources = "metal-cpp/*", "tests/*", "CMakeLists.txt", "LICENSE"
    def build_requirements(self):
        self.tool_requires("cmake/[^4.0.0]")
        self.tool_requires("ninja/[^1.3.0]")
        self.test_requires("boost-ext-ut/2.3.1")
        self.tool_requires("engine3d-cmake-utils/4.0")

    def requirements(self):
        # Put your dependencies here
        self.requires("glfw/3.4")
        self.requires("metal-cpp/15")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        llvm_path = self.dependencies.build["llvm-toolchain"].package_folder
        clang_tidy_path = f"{llvm_path}/bin/clang-tidy"

        self.output.info(f"{clang_tidy_path} found!")

        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_CLANG_TIDY"] = clang_tidy_path

        tc.generator = "Ninja"
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        copy(self, "LICENSE",
             dst=Path(self.package_folder) / "licenses",
             src=self.source_folder)

    def package_info(self):
        # DISABLE Conan's config file generation
        self.cpp_info.set_property("cmake_find_mode", "none")
        # Tell CMake to include this directory in its search path
        self.cpp_info.builddirs.append("lib/cmake")

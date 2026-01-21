from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class Demo(ConanFile):
    name = "demo"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    export_source = "CMakeLists.txt", "audio-cpp/*"

    # Putting all of your build-related dependencies here
    def build_requirements(self):
        self.tool_requires("cmake/[^4.0.0]")
        self.tool_requires("ninja/[^1.3.0]")
        self.test_requires("boost-ext-ut/2.3.1")
        self.tool_requires("engine3d-cmake-utils/4.0")


    # Putting all of your packages here
    def requirements(self):
        self.requires("library-template/1.0")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
    
    def layout(self):
        cmake_layout(self)

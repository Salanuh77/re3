"""Conan 2 dependencies for CI; librw is built from the pinned git submodule."""

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class Re3BuildDependencies(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    options = {"platform": ["GL3", "D3D9"]}
    default_options = {"platform": "GL3"}

    def requirements(self):
        self.requires("openal-soft/1.24.3")
        self.requires("mpg123/1.33.0")
        if self.options.platform == "GL3":
            self.requires("glfw/3.4")

    def generate(self):
        CMakeDeps(self).generate()
        toolchain = CMakeToolchain(self)
        toolchain.variables["LIBRW_PLATFORM"] = str(self.options.platform)
        toolchain.variables["LIBRW_GL3_GFXLIB"] = "GLFW"
        toolchain.variables["RE3_AUDIO"] = "OAL"
        toolchain.variables["RE3_VENDORED_LIBRW"] = True
        toolchain.variables["RE3_INSTALL"] = True
        toolchain.variables["BUILD_TESTING"] = False
        toolchain.user_presets_path = False
        toolchain.generate()

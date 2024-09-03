from conan import ConanFile
from conan.tools.meson import Meson
from conan.tools.files import copy


class Pkg(ConanFile):
    name = "forti_hole"
    version = "0.1.3"
    author = "Cooper Larson | cooper.larson1@gmail.com"
    url = ""
    description = "Pi-hole inspired DNS blacklisting tool for FortiGate external connector threat feeds."
    topics = ("c++", "security", "DNS blocklists", "Pi-hole")
    settings = "os", "compiler", "arch", "build_type"
    generators = "PkgConfigDeps", "MesonToolchain"
    exports_sources = "meson.build", "include/*", "main.cpp", "config.yaml"
    include = ['include']
    src = ['src']

    def layout(self):
        self.folders.source = '.'
        self.folders.build = 'build/meson'
        self.folders.generators = 'build/generators'
        self.folders.package = 'build/package'

    def requirements(self):
        self.requires('nlohmann_json/3.11.3')
        self.requires('forti-api/0.1.11')
        self.requires('yaml-cpp/0.8.0')
        self.requires('libcurl/8.9.1')
        self.test_requires('gtest/1.14.0')

    def build(self):
        meson = Meson(self)
        meson.configure()
        meson.build()

    def package(self):
        copy(self, "*.h", self.source_folder, self.package_folder)

    def package_id(self):
        self.info.clear()

import os
from conans import ConanFile

class LibeasyyamlConan(ConanFile):
    name = "libeasyyaml"
    version = "1.0.0"
    license = "MIT"
    author = "Michael Wright <mjw@methodanalysis.com>"
    url = "https://github.com/mwri/libeasyyaml",
    description = "YAML parsing in C with schemas, made easy",
    topics = ("yaml", "parse", "schema")
    settings = "os", "compiler", "build_type", "arch"

    def source(self):
        self.run("git clone https://github.com/mwri/libeasyyaml .")

    def build(self):
        self.run("libtoolize")
        self.run("autoreconf -i")
        self.run("./configure CFLAGS=-O3 --prefix=/usr/local")
        self.run("make")
        self.run("make test")
        self.run("make install DESTDIR="+os.getcwd()+"/install")

    def package(self):
        self.copy("*", src="install/usr/local/lib", dst="lib")
        self.copy("easyyaml.h", src="install/usr/local/include", dst="include")
        self.copy("LICENSE", src=".", dst=".")

from os.path import isdir, join
from platform import system, machine

from setuptools import Extension, find_packages, setup
from setuptools.command.build import build
from wheel.bdist_wheel import bdist_wheel


class Build(build):
    def run(self):
        if isdir("queries"):
            dest = join(self.build_lib, "tree_sitter_matlab", "queries")
            self.copy_tree("queries", dest)
        super().run()


class BdistWheel(bdist_wheel):
    def get_tag(self):
        python, abi, platform = super().get_tag()
        if python.startswith("cp"):
            python, abi = "cp38", "abi3"
        return python, abi, platform


def get_extra_compile_args():

    if system() == "Windows":
        return ["/std:c11", "/utf-8"]

    args = ["-std=c11"]

    arch = machine().lower()
    if "arm" in arch or "aarch64" in arch:
        if "aarch64" in arch or arch == "arm64":
            args.extend(["-march=armv8-a", "-mtune=generic"])
        else:
            args.extend(["-march=armv7-a", "-mtune=generic"])

    return args


setup(
    packages=find_packages("bindings/python"),
    package_dir={"": "bindings/python"},
    package_data={
        "tree_sitter_matlab": ["*.pyi", "py.typed"],
        "tree_sitter_matlab.queries": ["*.scm"],
    },
    ext_package="tree_sitter_matlab",
    ext_modules=[
        Extension(
            name="_binding",
            sources=[
                "bindings/python/tree_sitter_matlab/binding.c",
                "src/parser.c",
                "src/scanner.c"
            ],
            extra_compile_args=get_extra_compile_args(),
            define_macros=[
                ("Py_LIMITED_API", "0x03080000"),
                ("PY_SSIZE_T_CLEAN", None)
            ],
            include_dirs=["src"],
            py_limited_api=True,
        )
    ],
    cmdclass={
        "build": Build,
        "bdist_wheel": BdistWheel
    },
    zip_safe=False
)

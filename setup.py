import sys

from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup

import ctypes as ct  # to call native
import ctypes.util as ctu
import platform  # to learn the OS we're on

extra_compile_args = ["-Ilibs/third_party", "-Iinclude", "-Iqasmtools/include"]
extra_links_args = []


def _load_shared_obj(name):
    """Attempts to load shared library."""
    paths = []

    # search typical locations
    paths += [ctu.find_library(name)]
    paths += [ctu.find_library("lib" + name)]
    dll = ct.windll if platform.system() == "Windows" else ct.cdll

    for path in paths:
        if path:
            lib = dll.LoadLibrary(path)
            return lib

    raise RuntimeError("No " + name + " shared libraries found")


found_GMP = True
try:
    _libgmp = _load_shared_obj("gmp")
    _libgmpxx = _load_shared_obj("gmpxx")
except:
    found_GMP = False

if found_GMP:
    extra_compile_args.append("-DGRID_SYNTH")
    extra_compile_args.append("-DEXPR_GMP")
    extra_links_args = ["-lgmp", "-lgmpxx"]

# If the platform seem to be MSVC
if sys.platform == "win32" and not sys.platform == "cygwin" and not sys.platform == "msys":
    extra_compile_args.append("-Ilibs/third_party/pthreadwin32")

ext_modules = [
    Pybind11Extension(
        "pystaq",
        ["pystaq/staq_wrapper.cpp"],
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_links_args,
        cxx_std=17,
        include_pybind11=False,
    ),
]

setup(platforms=sys.platform,
      ext_modules=ext_modules)

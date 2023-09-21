import sys

from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup

extra_compile_args = ["-Ilibs", "-Iinclude", "-Iqasmtools/include"]
# Compile with qasm_synth for now
extra_compile_args.append("-DQASM_SYNTH")
extra_compile_args.append("-DEXPR_GMP")
extra_compile_args.append("-lgmpxx")
extra_compile_args.append("-lgmp")
extra_compile_args.append("-L/usr/lib/x86_64-linux-gnu")

# If the platform seem to be MSVC
if sys.platform == "win32" and not sys.platform == "cygwin" and not sys.platform == "msys":
    extra_compile_args.append("-Ilibs/pthreadwin32")

ext_modules = [
    Pybind11Extension(
        "pystaq",
        ["pystaq/staq_wrapper.cpp"],
        extra_compile_args=extra_compile_args,
        cxx_std=17,
        include_pybind11=False,
    ),
]

setup(platforms=sys.platform,
      ext_modules=ext_modules)

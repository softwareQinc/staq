from setuptools import setup
from libs.pybind11.setup_helpers import Pybind11Extension

ext_modules = [
    Pybind11Extension(
        "pystaq",
        ["pystaq/staq_wrapper.cpp"],
        extra_compile_args=["-Ilibs", "-Iinclude", "-Iqasmtools/include"],
        cxx_std=17,
        include_pybind11=False,
    ),
]

setup(
    name='pystaq',
    version='2.1',
    description='Python wrapper for staq',
    author='softwareQ',
    author_email='info@softwareq.ca',
    url='https://github.com/softwareQinc/staq',
    ext_modules=ext_modules)

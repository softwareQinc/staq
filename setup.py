import sys

from pybind11.setup_helpers import Pybind11Extension
from setuptools import setup

extra_compile_args = ["-Ilibs", "-Iinclude", "-Iqasmtools/include"]

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

setup(
    name='pystaq',
    version='3.0.1',
    description='Python 3 wrapper for staq',
    long_description=open('pystaq/README.md').read(),
    long_description_content_type='text/markdown',
    author='softwareQ',
    author_email='info@softwareq.ca',
    url='https://github.com/softwareQinc/staq',
    license='MIT',
    platforms=sys.platform,
    install_requires=[
        'pybind11',
    ],
    ext_modules=ext_modules)

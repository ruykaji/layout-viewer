from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext
import numpy

extra_compile_args = []
extra_link_args = []

extra_compile_args.append('-fopenmp')
extra_link_args.append('-fopenmp')

ext_modules = [
    Pybind11Extension(
        'net_connectivity',
        ['net_connectivity.cpp'],
        include_dirs=[numpy.get_include()],
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
    ),
]

setup(
    name="net_connectivity",
    version="0.0.3",
    author="",
    author_email="",
    description="",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)

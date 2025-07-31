from setuptools import setup, Extension

module = Extension("cstats", sources=["cstats.c"])

setup(
    name="cstats",
    version="1.0",
    description="C extension module for fast statistics",
    ext_modules=[module],
)

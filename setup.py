from distutils.core import setup, Extension

setup(
    name="bitarray", 
    author="Stephen M. Herwig",
    author_email="smherwig@cs.umd.edu",
    version="0.1",
    description="Bitarray C-extension",
    ext_modules=[Extension("bitarray", ["bitarraymodule.c"])]
    )

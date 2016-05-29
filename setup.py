#!/usr/bin/env python


from setuptools import setup, find_packages, Extension
import subprocess
import os
from distutils import ccompiler

def version_scheme(version):
    from setuptools_scm.version import guess_next_dev_version
    version = guess_next_dev_version(version)
    return version.lstrip("v")

LZ4_VERSION = "r131"

# Check to see if we have a lz4 library installed on the system and
# use it if so. If not, we'll use the bundled library. If lz4 is
# installed it will have a pkg-config file, so we'll use pkg-config to
# check for existence of the library.
try:
    pkg_config_exe = os.environ.get('PKG_CONFIG', None) or 'pkg-config'
    cmd = '{0} --exists liblz4'.format(pkg_config_exe).split()
    liblz4_found = subprocess.call(cmd) == 0
except OSError: 
    # pkg-config not present
    liblz4_found = False

if liblz4_found:
    # Use system lz4, and don't set optimization and warning flags for
    # the compiler. Specifically we don't define LZ4_VERSION since the
    # system lz4 library could be updated (that's the point of a
    # shared library).
    if ccompiler.get_default_compiler() == "msvc":
        extra_compile_args = ["/Ot", "/Wall"]
    else:
        extra_compile_args = ["-std=c99",]

    lz4mod = Extension('block',
                       [
                           'lz4/block.c'
                       ],
                       extra_compile_args=extra_compile_args,
                       define_macros=define_macros,
                       libraries=['lz4'],
    )
else:
    # Use the bundled lz4 libs, and set the compiler flags as they
    # historically have been set. We do set LZ4_VERSION here, since it
    # won't change after compilation.
    if ccompiler.get_default_compiler() == "msvc":
        extra_compile_args = ["/Ot", "/Wall"]
        define_macros = [("LZ4_VERSION","\\\"%s\\\"" % LZ4_VERSION),]
    else:
        extra_compile_args = ["-std=c99","-O3","-Wall","-W","-Wundef"]
        define_macros = [("LZ4_VERSION","\"%s\"" % LZ4_VERSION),]

    lz4mod = Extension('block',
                       [
                           'lz4libs/lz4.c',
                           'lz4libs/lz4hc.c',
                           'lz4/block.c'
                       ],
                       extra_compile_args=extra_compile_args,
                       define_macros=define_macros,
                       include_dirs=['lz4libs',],
    )


setup(
    name='lz4',
    use_scm_version={
        'write_to': "lz4/version.py",
        'version_scheme': version_scheme,
    },
    setup_requires=['setuptools_scm'],
    description="LZ4 Bindings for Python",
    long_description=open('README.rst', 'r').read(),
    author='Steeve Morin',
    author_email='steeve.morin@gmail.com',
    url='https://github.com/python-lz4/python-lz4',
    packages=find_packages('lz4'),
    #package_dir={'': 'lz4'},
    ext_package='lz4',
    ext_modules=[lz4mod,],
    tests_require=["nose>=1.0"],
    test_suite = "nose.collector",
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'License :: OSI Approved :: BSD License',
        'Intended Audience :: Developers',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
    ],
)

#!/usr/bin/env python
from setuptools import setup, find_packages, Extension
import sys
from distutils import ccompiler

# Note: if updating LZ4_REQUIRED_VERSION you need to update docs/install.rst as
# well.
LZ4_REQUIRED_VERSION = '>= 1.7.5'
PY3C_REQUIRED_VERSION = '>= 1.0'

# Check to see if we have a lz4 and py3c libraries installed on the system, and
# of suitable versions, and use if so. If not, we'll use the bundled libraries.
liblz4_found = False
py3c_found = False
try:
    import pkgconfig
except ImportError:
    # pkgconfig is not installed. It will be installed by setup_requires.
    pass
else:
    def pkgconfig_installed(lib, required_version, default):
        installed = default
        try:
            installed = pkgconfig.installed(lib, required_version)
        except EnvironmentError:
            # Windows, no pkg-config present
            pass
        except ValueError:
            # pkgconfig was unable to determine if
            # required version of liblz4 is available
            # Bundled version of liblz4 will be used
            pass
        return installed
    liblz4_found = pkgconfig_installed('liblz4', LZ4_REQUIRED_VERSION, default=False)
    py3c_found = pkgconfig_installed('py3c', PY3C_REQUIRED_VERSION, default=False)


# Set up the extension modules. If a system wide lz4 library is found, and is
# recent enough, we'll use that. Otherwise we'll build with the bundled one. If
# we're building against the system lz4 library we don't set the compiler
# flags, so they'll be picked up from the environment. If we're building
# against the bundled lz4 files, we'll set the compiler flags to be consistent
# with what upstream lz4 recommends.

include_dirs = []
libraries = []

lz4version_sources = [
    'lz4/_version.c'
]

lz4block_sources = [
    'lz4/block/_block.c'
]

lz4frame_sources = [
    'lz4/frame/_frame.c'
]

if liblz4_found is True:
    libraries.append('lz4')
else:
    include_dirs.append('lz4libs')
    lz4version_sources.extend(
        [
            'lz4libs/lz4.c',
        ]
    )
    lz4block_sources.extend(
        [
            'lz4libs/lz4.c',
            'lz4libs/lz4hc.c',
        ]
    )
    lz4frame_sources.extend(
        [
            'lz4libs/lz4.c',
            'lz4libs/lz4hc.c',
            'lz4libs/lz4frame.c',
            'lz4libs/xxhash.c',
        ]
    )

if py3c_found is False:
    include_dirs.append('py3c')

compiler = ccompiler.get_default_compiler()

extra_link_args = []
extra_compile_args = []

if compiler == 'msvc':
    extra_compile_args = ['/Ot', '/Wall']
elif compiler in ('unix', 'mingw32'):
    if liblz4_found:
        extra_link_args.append(pkgconfig.libs('liblz4'))
        if pkgconfig.cflags('liblz4'):
            extra_compile_args.append(pkgconfig.cflags('liblz4'))
    else:
        extra_compile_args = [
            '-O3',
            '-Wall',
            '-Wundef'
        ]
else:
    print('Unrecognized compiler: {0}'.format(compiler))
    sys.exit(1)

lz4version = Extension('lz4._version',
                       lz4version_sources,
                       extra_compile_args=extra_compile_args,
                       extra_link_args=extra_link_args,
                       libraries=libraries,
                       include_dirs=include_dirs)

lz4block = Extension('lz4.block._block',
                     lz4block_sources,
                     extra_compile_args=extra_compile_args,
                     extra_link_args=extra_link_args,
                     libraries=libraries,
                     include_dirs=include_dirs)

lz4frame = Extension('lz4.frame._frame',
                     lz4frame_sources,
                     extra_compile_args=extra_compile_args,
                     extra_link_args=extra_link_args,
                     libraries=libraries,
                     include_dirs=include_dirs)

install_requires = []

# On Python earlier than 3.0 the builtins package isn't included, but it is
# provided by the future package
if sys.version_info < (3, 0):
    install_requires.append('future')


# Dependencies for testing. We define a list here, so that we can
# refer to it for the tests_require and the extras_require arguments
# to setup below. The latter enables us to use pip install .[tests] to
# install testing dependencies.
tests_require = [
    'pytest',
    'psutil',
],

# Finally call setup with the extension modules as defined above.
setup(
    name='lz4',
    use_scm_version={
        'write_to': "lz4/version.py",
    },
    python_requires=">=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*",
    setup_requires=[
        'setuptools_scm',
        'pkgconfig',
        'pytest-runner',
    ],
    install_requires=install_requires,
    description="LZ4 Bindings for Python",
    long_description=open('README.rst', 'r').read(),
    author='Jonathan Underwood',
    author_email='jonathan.underwood@gmail.com',
    url='https://github.com/python-lz4/python-lz4',
    packages=find_packages(),
    ext_modules=[
        lz4version,
        lz4block,
        lz4frame
    ],
    tests_require=tests_require,
    extras_require={
        'tests': tests_require,
        'flake8': [
            'flake8',
        ]
    },
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'License :: OSI Approved :: BSD License',
        'Intended Audience :: Developers',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
    ],
)

#!/usr/bin/env python


from setuptools import setup, find_packages, Extension

VERSION = (0, 6, 1)

setup(
    name='lz4',
    version=".".join([str(x) for x in VERSION]),
    description="LZ4 Bindings for Python",
    long_description=open('README.rst', 'r').read(),
    author='Steeve Morin',
    author_email='steeve.morin@gmail.com',
    url='https://github.com/steeve/python-lz4',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    ext_modules=[
        Extension('lz4', [
            'src/lz4.c',
            'src/lz4hc.c',
            'src/python-lz4.c'
        ], extra_compile_args=["-O4"])
    ],
    setup_requires=["nose>=1.0"],
    test_suite = "nose.collector",
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'License :: OSI Approved :: BSD 3-Clause License',
        'Intended Audience :: Developers',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.3',
    ],
)

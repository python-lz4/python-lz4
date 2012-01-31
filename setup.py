#!/usr/bin/env python

"""Setup file for veezio backend"""

from setuptools import setup, find_packages, Extension

setup(
    name='lz4',
    version='0.1',
    description=open('README.rst', 'r').read(),
    author='Steeve Morin',
    author_email='steeve.morin@gmail.com',
    url='https://github.com/steeve/python-lz4',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    ext_modules=[
        Extension('lz4', [
            'src/lz4.c',
            'src/python-lz4.c'
        ])
    ],
)

==========
python-lz4
==========

.. image:: https://secure.travis-ci.org/python-lz4/python-lz4.png?branch=master

.. image:: https://readthedocs.org/projects/python-lz4/badge/?version=latest
:target: http://python-lz4.readthedocs.io/en/latest/?badge=latest
:alt: Documentation Status

Introduction
============
This package provides python bindings for the `lz4 compression library
<https://cyan4973.github.io/lz4//>`_ by Yann Collet.

At this time the project contains bindings for the LZ4 block format
and the LZ4 frame format. Patches implementing bindings for the LZ4
stream format would be readily accepted!

Install
=======
The package is hosted on `PyPI <http://pypi.python.org/pypi/lz4>`_::

    $ pip install lz4
    $ easy_install lz4

Documenation
============
Full documentation is included with the project. The documentation is
generated using Sphinx.

Licensing
=========
Code specific to this project is covered by the `BSD 3-Clause License
<http://opensource.org/licenses/BSD-3-Clause>`_


Contributors
============
- Jonathan Underwood combined the block and frame modules into a
  coherent single project with many fixes and cleanups including
  updating the block format support to use the tunable accelerated and
  high compression functinos
- Steve Morin wrote the original lz4 block bindings
- Christopher Jackson wrote the original lz4 frame bindings as part of
  the `lz4tools <https://github.com/darkdragn/lz4tools>`_ project
- Mathew Rocklin added support for dropping the GIL to the block
  module, Travis testing support
- Antoine Martin added initial support for fast compression support in
  the block library


	   
.. toctree::
   docs/intro.rst
   docs/install.rst
   docs/license.rst
   docs/contributors.rst


.. image:: https://cruel-carlota.pagodabox.com/d37459f4fce98f2983589a1c1c23a4e4

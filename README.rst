==========
python-lz4
==========

.. image:: https://secure.travis-ci.org/python-lz4/python-lz4.png?branch=master

Overview
========
This package provides bindings for the `lz4 compression library
<https://cyan4973.github.io/lz4//>`_ by Yann Collet.

Code specific to this project is covered by the `BSD 3-Clause License
<http://opensource.org/licenses/BSD-3-Clause>`_

Install
=======
The package is hosted on `PyPI <http://pypi.python.org/pypi/lz4>`_::

    $ pip install lz4
    $ easy_install lz4

Usage
=====
At this time the project contains bindings for the LZ4 block format
and the LZ4 frame format. Patches implementing bindings for the LZ4
stream format would be readily accepted!

Block bindings
--------------
Because the LZ4 block format doesn't define a container format, the
python bindings will insert the original data size as an integer at
the start of the compressed payload, like most other bindings do
(Java...)

To use the lz4 block format bindings is straightforward::

    >>> import lz4.block
    >>> compressed_data = lz4.block.compress(data)
    >>> data == lz4.block.decompress(compressed_data)
    True
    >>>

Methods
~~~~~~~
The block module provides two methods, ``compress`` and
``decompress``.

``compress(source, mode='default', acceleration=1, compression=0)``

This function compresses ``source``. If ``mode`` is not specified it
will use 

Deprecated methods
~~~~~~~~~~~~~~~~~~
The following methods are provided as wrappers around ``compress`` and
``decompress`` for backwards compatibility, but will be removed in the
near future. These methods are also imported into the lz4 top level
namespace for backwards compatibility.

- ``dumps``, and ``LZ4_compress`` are wrappers around ``compress``
- ``loads``, ``uncompress``, ``LZ4_uncompress``are wrappers around
  ``decompress``
- ``compress_fast`` and ``LZ4_compress_fast`` are wrappers around
  ``compress`` with ``mode=fast``
- ``compressHC`` is a wrapper around ``compress`` with
  ``mode=high_compression``

Is it fast ?
============
Yes. Here are the results on my 2011 Macbook Pro i7 with lz4.c as input data: ::

    $ python tests/bench.py
    Data Size:
      Input: 24779
      LZ4: 10152 (0.41)
      Snappy: 9902 (0.40)
      LZ4 / Snappy: 1.025247
    Benchmark: 200000 calls
      LZ4 Compression: 9.737272s
      Snappy Compression: 18.012336s
      LZ4 Decompression: 2.686854s
      Snappy Decompression : 5.146867s


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


.. image:: https://cruel-carlota.pagodabox.com/d37459f4fce98f2983589a1c1c23a4e4

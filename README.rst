==========
python-lz4
==========

.. image:: https://secure.travis-ci.org/steeve/python-lz4.png?branch=master

Overview
========
This package provides bindings for the `lz4 compression library <http://code.google.com/p/lz4/>`_ by Yann Collet.

Code specific to this project is covered by the `BSD 3-Clause License <http://opensource.org/licenses/BSD-3-Clause>`_

Install
=======
The package is hosted on `PyPI <http://pypi.python.org/pypi/lz4>`_::

    $ pip install lz4
    $ easy_install lz4

Usage
=====
The library is pretty simple to use::

    >>> import lz4
    >>> compressed_data = lz4.dumps(data)
    >>> data == lz4.loads(compressed_data)
    True
    >>>

Methods
=======
The bindings provides some aliases too::

    >>> import lz4
    >>> lz4.LZ4_compress == lz4.compress == lz4.dumps
    True
    >>> lz4.LZ4_uncompress == lz4.uncompress == z4.decompress == lz4.loads
    True
    >>>

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

Important note
==============
Because LZ4 doesn't define a container format, the python bindings will insert the original data size as an integer at the start of the compressed payload, like most bindings do anyway (Java...)

.. image:: https://cruel-carlota.pagodabox.com/d37459f4fce98f2983589a1c1c23a4e4

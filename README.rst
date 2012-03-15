==========
python-lz4
==========

.. image:: https://secure.travis-ci.org/steeve/python-lz4.png?branch=master

Overview
========
This package provides bindings for the `lz4 compression library <http://code.google.com/p/lz4/>`_ by Yann Collet.

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
    >>> lz4.LZ4_uncompress == lz4.uncompress == lz4.loads
    True
    >>>

Is it fast ?
============
Yes. Here are the results on my 2011 Macbook Pro i7 with 128kb random data: ::

    $ python tests/bench.py
    Data Size:
      Input: 131072
      LZ4: 131601
      Snappy: 131087
      LZ4 / Snappy: 1.003921
    Benchmark: 200000 calls
      LZ4 Compression: 3.651002s
      Snappy Compression: 8.066482s
      LZ4 Decompression: 1.482934s
      Snappy Decompression : 3.193481s

Important note
==============
Because LZ4 doesn't define a container format, the python bindings will insert the original data size as an integer at the start of the compressed payload, like most bindings do anyway (Java...)

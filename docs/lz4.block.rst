.. _lz4.block:

lz4.block sub-package
=====================

This sub-package provides the capability to compress and decompress data using
the _`LZ4 block specification <http://lz4.github.io/lz4/lz4_Block_format.html>`

Because the LZ4 block format doesn't define a container format, the Python
bindings will by default insert the original data size as an integer at the
start of the compressed payload, like most other bindings do (Java...). However,
it is possible to disable this functionality.

Example usage
-------------
To use the lz4 block format bindings is straightforward::

    >>> import lz4.block
    >>> compressed_data = lz4.block.compress(data)
    >>> data == lz4.block.decompress(compressed_data)
    True
    >>>

Contents
----------------

.. automodule:: lz4.block
    :members: compress, decompress
    :undoc-members:
    :show-inheritance:


Is it fast ?
------------
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

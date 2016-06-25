lz4.block package
=================

This subpackage provides wrappers around the LZ4 block format library
functions. More detail can be found in the article `LZ4
explained<http://fastcompression.blogspot.co.uk/2011/05/lz4-explained.html>`_
by the author of the LZ4 library.

Because the LZ4 block format doesn't define a container format, the
python bindings will insert the original data size as an integer at
the start of the compressed payload, like most other bindings do
(Java...)

Example usage
-------------
To use the lz4 block format bindings is straightforward::

    >>> import lz4.block
    >>> compressed_data = lz4.block.compress(data)
    >>> data == lz4.block.decompress(compressed_data)
    True
    >>>

Module contents
---------------

.. automodule:: lz4.block
    :members: compress, decompress
    :undoc-members:
    :show-inheritance:

Deprecated methods
------------------
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

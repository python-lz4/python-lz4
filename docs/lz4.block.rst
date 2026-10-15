.. default-role:: obj


lz4.block sub-package
=====================

This sub-package provides the capability to compress and decompress data using
the `block specification <https://lz4.github.io/lz4/lz4_Block_format.html>`_.

Because the LZ4 block format doesn't define a container format, the
Python bindings will by default insert the original data size as an
integer at the start of the compressed payload. However, it is
possible to disable this functionality, and you may wish to do so for
compatibility with other language bindings, such as the `Java bindings
<https://github.com/lz4/lz4-java>`_.



Example usage
-------------
To use the lz4 block format bindings is straightforward:

.. doctest::

   >>> import lz4.block
   >>> import os
   >>> input_data = 20 * 128 * os.urandom(1024)  # Read 20 * 128kb
   >>> compressed_data = lz4.block.compress(input_data)
   >>> output_data = lz4.block.decompress(compressed_data)
   >>> input_data == output_data
   True

In this simple example, the size of the uncompressed data is stored in
the compressed data, and this size is then utilized when uncompressing
the data in order to correctly size the buffer. Instead, you may want
to not store the size of the uncompressed data to ensure compatibility
with the `Java bindings <https://github.com/lz4/lz4-java>`_. The
example below demonstrates how to use the block format without storing
the size of the uncompressed data.

.. doctest::

   >>> import lz4.block
   >>> data = b'0' * 255
   >>> compressed = lz4.block.compress(data, store_size=False)
   >>> decompressed = lz4.block.decompress(compressed, uncompressed_size=255)
   >>> decompressed == data
   True

The `uncompressed_size` argument specifies an upper bound on the size
of the uncompressed data size rather than an absolute value, such that
the following example also works.

.. doctest::
   
   >>> import lz4.block
   >>> data = b'0' * 255
   >>> compressed = lz4.block.compress(data, store_size=False)
   >>> decompressed = lz4.block.decompress(compressed, uncompressed_size=2048)
   >>> decompressed == data
   True

A common situation is not knowing the size of the uncompressed data at
decompression time. The following example illustrates a strategy that
can be used in this case.

.. doctest::
   
   >>> import lz4.block
   >>> data = b'0' * 2048
   >>> compressed = lz4.block.compress(data, store_size=False)
   >>> usize = 255
   >>> max_size = 4096
   >>> while True:
   ...     try:
   ...         decompressed = lz4.block.decompress(compressed, uncompressed_size=usize)
   ...         break
   ...     except lz4.block.LZ4BlockError:
   ...         usize *= 2
   ...         if usize > max_size:
   ...             print('Error: data too large or corrupt')
   ...             break
   >>> decompressed == data
   True

In this example we are catching the `lz4.block.LZ4BlockError`
exception. This exception is raised if the LZ4 library call fails,
which can be caused by either the buffer used to store the
uncompressed data (as set by `usize`) being too small, or the input
compressed data being invalid - it is not possible to distinguish the
two cases, and this is why we set an absolute upper bound (`max_size`)
on the memory that can be allocated for the uncompressed data. If we
did not take this precaution, the code, if ppassed invalid compressed
data would continuously try to allocate a larger and larger buffer for
decompression until the system ran out of memory.
   
Compressing into a fixed-size block
----------------------------------

``compress_dest_size`` wraps ``LZ4_compress_destSize``. It compresses as much of
its input as fits in ``target_size`` bytes and returns both the compressed block
and the number of source bytes consumed. Unlike ``compress`` with its default
settings, it does not prepend an uncompressed-size header.

.. doctest::

   >>> source = bytes(range(256)) * 4
   >>> block, consumed = lz4.block.compress_dest_size(source, target_size=64)
   >>> len(block) <= 64 and 0 < consumed < len(source)
   True
   >>> lz4.block.decompress(block, uncompressed_size=consumed) == source[:consumed]
   True

To compress the rest, call it again with ``source[consumed:]``. Each result is an
independent block; keep its consumed count for decompression. A very small target
can produce an empty block while consuming zero input bytes. Check for this
before advancing through input, and increase the target size if necessary.
``return_bytearray=True`` selects a mutable compressed result.

This function requires LZ4 1.9.2 or newer and raises ``RuntimeError`` with older
libraries. Earlier LZ4 versions have a known ``LZ4_compress_destSize`` issue that
can require an extra byte in the decompression buffer. The existing compression
and decompression APIs retain their library-version requirements.

Contents
----------------

.. automodule:: lz4.block
    :members: compress, compress_dest_size, decompress


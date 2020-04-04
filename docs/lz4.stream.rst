.. default-role:: obj


lz4.stream sub-package
======================

This sub-package provides the capability to compress and decompress data using
the `stream specification
<https://github.com/lz4/lz4/blob/master/examples/streaming_api_basics.md>`_,
especially the `stream specification based on a double buffer
<https://github.com/lz4/lz4/blob/master/examples/blockStreaming_doubleBuffer.md>`_.

Because the LZ4 stream format does not define a container format, the
Python bindings will by default insert the compressed data size as an
integer at the start of the compressed payload. However, it is
possible to set the bit depth of this compressed data size.

So far, only the double-buffer based approach is implemented.

Example usage
-------------
To use the lz4 stream format bindings is straightforward:

.. doctest::

   >>> from lz4.stream import LZ4StreamCompressor, LZ4StreamDecompressor
   >>> import os
   >>> block_size_length = 2 # LZ4 compressed block size stored on 2 bytes
   >>> page_size = 8192 # LZ4 context double buffer page size
   >>> origin_stream = 10 * 1024 * os.urandom(1024) # 10MiB
   >>> # LZ4 stream compression of origin_stream into compressed_stream:
   >>> compressed_stream = bytearray()
   >>> with LZ4StreamCompressor("double_buffer", page_size, store_comp_size=block_size_length) as proc:
   ...     offset = 0
   ...     while offset < len(origin_stream):
   ...         chunk = origin_stream[offset:offset + page_size]
   ...         block = proc.compress(chunk)
   ...         compressed_stream.extend(block)
   ...         offset += page_size
   >>> # LZ4 stream decompression of compressed_stream into decompressed_stream:
   >>> decompressed_stream = bytearray()
   >>> with LZ4StreamDecompressor("double_buffer", page_size, store_comp_size=block_size_length) as proc:
   ...     offset = 0
   ...     while offset < len(compressed_stream):
   ...         block = proc.get_block(compressed_stream[offset:])
   ...         chunk = proc.decompress(block)
   ...         decompressed_stream.extend(chunk)
   ...         offset += block_size_length + len(block)
   >>> decompressed_stream == origin_stream
   True

Out-of-band block size record example
-------------------------------------
.. doctest::

   >>> from lz4.stream import LZ4StreamCompressor, LZ4StreamDecompressor
   >>> import os
   >>> page_size = 8192 # LZ4 context double buffer page size
   >>> out_of_band_block_sizes = [] # Store the block sizes
   >>> origin_stream = 10 * 1024 * os.urandom(1024) # 10MiB
   >>> # LZ4 stream compression of origin_stream into compressed_stream:
   >>> compressed_stream = bytearray()
   >>> with LZ4StreamCompressor("double_buffer", page_size, store_comp_size=0) as proc:
   ...     offset = 0
   ...     while offset < len(origin_stream):
   ...         chunk = origin_stream[offset:offset + page_size]
   ...         block = proc.compress(chunk)
   ...         out_of_band_block_sizes.append(len(block))
   ...         compressed_stream.extend(block)
   ...         offset += page_size
   >>> # LZ4 stream decompression of compressed_stream into decompressed_stream:
   >>> decompressed_stream = bytearray()
   >>> with LZ4StreamDecompressor("double_buffer", page_size, store_comp_size=0) as proc:
   ...     offset = 0
   ...     for block_len in out_of_band_block_sizes:
   ...         # Sanity check:
   ...         if offset >= len(compressed_stream):
   ...             raise LZ4StreamError("Truncated stream")
   ...         block = compressed_stream[offset:offset + block_len]
   ...         chunk = proc.decompress(block)
   ...         decompressed_stream.extend(chunk)
   ...         offset += block_len
   >>> decompressed_stream == origin_stream
   True

Contents
----------------

.. automodule:: lz4.stream
    :members: LZ4StreamCompressor, LZ4StreamDecompressor

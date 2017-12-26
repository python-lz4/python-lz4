Quickstart
==========

The recommended binding to use is the LZ4 frame format binding, as this provides
interoperability with other implementations.

The simplest way to use the frame bindings is via the compress and decompress
functions::

  >>> import os
  >>> import lz4.frame
  >>> input_data = 20 * 128 * os.urandom(1024)  # Read 20 * 128kb
  >>> compressed = lz4.frame.compress(input_data)
  >>> decompressed = lz4.frame.decompress(compressed)
  >>> decompressed == input_data
  Out[6]: True

The ``compress`` function reads the input data and compresses it and returns a
LZ4 frame. A frame consists of a header, and a sequence of blocks of compressed
data, and a frame end marker (and optionally a checksum of the uncompressed
data). The ``decompress`` function takes a full LZ4 frame, decompresses it (and
optionally verifies the uncompressed data against the stored checksum), and
returns the uncompressed data.

It's often inconvenient to hold the full data in memory, and so functions are
also provided to compress and decompress data in chunks::

  >>> import lz4.frame
  >>> import os
  >>> input_data = 20 * 128 * os.urandom(1024)
  >>> c_context = lz4.frame.create_compression_context()
  >>> compressed = lz4.frame.compress_begin(c_context)
  >>> compressed += lz4.frame.compress_chunk(c_context, input_data[:10 * 128 * 1024])
  >>> compressed += lz4.frame.compress_chunk(c_context, input_data[10 * 128 * 1024:])
  >>> compressed += compress_flush(c_context)

Here a compression context is first created which is used to maintain state
across calls to the LZ4 library. This is an opaque PyCapsule object.
``compress_begin`` starts a new frame and returns the frame header.
``compress_chunk`` compresses input data and returns the compressed data.
``compress_flush`` ends the frame and returns the frame end marker.
``compress_flush`` also flushes any buffered data. By default,
``compress_chunk`` may buffer data until a block is full. This buffering can be
disabled by specifying ``auto_flush=True`` when calling ``compress_begin``.
Alternatively, the LZ4 buffers can be flushed at any time without ending the
frame by calling ``compress_flush`` with ``end_frame=False``.

Decompressing data can also be done in a chunked fashion::

  >>> d_context = lz4.frame.create_decompression_context()
  >>> decompressed = lz4.frame.decompress_chunk(d_context, compressed[:len(compressed)//2])
  >>> decompressed += lz4.frame.decompress_chunk(d_context, compressed[len(compressed)//2:])
  >>> decompressed == input_data
  Out[12]: True

Rather than managing compression and decompression context objects manually, it
is more convenient to use the ``LZ4FrameCompressor`` and
``LZ4FrameDecompressor`` classes which provide context manager functionality::

  >>> import lz4.frame
  >>> import os
  >>> input_data = 20 * 128 * os.urandom(1024)
  >>> with lz4.frame.LZ4FrameCompressor() as compressor:
  ...     compressed = compressor.begin()
  ...     compressed += compressor.compress(input_data[:10 * 128 * 1024])
  ...     compressed += compressor.compress(input_data[10 * 128 * 1024:])
  ...     compressed += compressor.finalize()
  >>> with lz4.frame.LZ4FrameDecompressor() as decompressor:
  ...     decompressed = decompressor.decompress(compressed[:len(compressed)//2])
  ...     decompressed += decompressor.decompress(compressed[len(compressed)//2:])
  >>> decompressed == input_data
  Out[13]: True

Controlling the compression
---------------------------
Beyond the basics, there are a number of options to play with to tune and
control the compression. A few of the key ones are listed below, please see the
documentation for full details of options.


``compression_level``
~~~~~~~~~~~~~~~~~~~~~

This specifies the level of compression used with 0 (default) being the lowest
compression (0-2 are the same value), and 16 the highest compression. Values
below 0 will enable "fast acceleration", proportional to the value. Values above
16 will be treated as 16. The following module constants are provided as a
convenience:

- ``lz4.frame.COMPRESSIONLEVEL_MIN``: Minimum compression (0, default)
- ``lz4.frame.COMPRESSIONLEVEL_MINHC``: Minimum high-compression mode (3)
- ``lz4.frame.COMPRESSIONLEVEL_MAX``: Maximum compression (16)

Availability: ``lz4.frame.LZ4FrameCompressor()``, ``lz4.frame.compress()``,
``lz4.frame.compress_begin()``


``block_size``
~~~~~~~~~~~~~~
This specifies the maximum blocksize to use for the blocks in a frame. Options:

- ``lz4.frame.BLOCKSIZE_DEFAULT`` or 0: the lz4 library default
- ``lz4.frame.BLOCKSIZE_MAX64KB`` or 4: 64 kB
- ``lz4.frame.BLOCKSIZE_MAX256KB`` or 5: 256 kB
- ``lz4.frame.BLOCKSIZE_MAX1MB`` or 6: 1 MB
- ``lz4.frame.BLOCKSIZE_MAX4MB`` or 7: 4 MB

If unspecified, will default to ``lz4.frame.BLOCKSIZE_DEFAULT`` which is
currently equal to ``lz4.frame.BLOCKSIZE_MAX64KB``

Availability: ``lz4.frame.LZ4FrameCompressor()``, ``lz4.frame.compress()``,
   ``lz4.frame.compress_begin()``


``block_linked``
~~~~~~~~~~~~~~~~

This specifies whether to use block-linked compression. If ``True``, the
compression ratio is improved, particularly for small block sizes. Default is
``True``.

Availability: ``lz4.frame.LZ4FrameCompressor()``, ``lz4.frame.compress()``,
``lz4.frame.compress_begin()``


``content_checksum``
~~~~~~~~~~~~~~~~~~~~

This specifies whether to enable checksumming of the uncompressed content. If
``True``, a checksum is stored at the end of the frame, and checked during
decompression. Default is ``False``.

Availability: ``lz4.frame.LZ4FrameCompressor()``, ``lz4.frame.compress()``,
``lz4.frame.compress_begin()``


``block_checksum``
~~~~~~~~~~~~~~~~~~

This specifies whether to enable checksumming of the uncompressed content of
each block in the frame. If ``True``, a checksum is stored at the end of each
block in the frame, and checked during decompression. Default is ``False``.

Availability: ``lz4.frame.LZ4FrameCompressor()``, ``lz4.frame.compress()``,
``lz4.frame.compress_begin()``


``auto_flush``
~~~~~~~~~~~~~~

Enable or disable autoFlush. When autoFlush is disabled, the LZ4 library may
buffer data internally until block is full. Default is ``False`` (autoFlush
disabled).

Availability: ``lz4.frame.LZ4FrameCompressor()``, ``lz4.frame.compress_begin()``


``store_size`` and ``source_size``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These allow for storing the size of the uncompressed data in the frame header.
Storing the source size in the frame header adds an extra 8 bytes to the size of
the compressed frame, but allows the decompression functions to better size
memory buffers.

**``store_size``**
If ``store_size`` is ``True`` the size of the uncompressed data will be stored in
the frame header for use during decompression. Default is ``True``.

Availability: ``lz4.frame.compress()``


**``source_size``** This optionally specifies the uncompressed size of the source
 data to be compressed. If specified, the size will be stored in the frame
 header for use during decompression.

Availability: ``lz4.frame.LZ4FrameCompressor.begin()``,
   ``lz4.frame.compress_begin()``


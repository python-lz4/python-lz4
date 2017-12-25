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

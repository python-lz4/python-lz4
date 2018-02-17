.. py:currentmodule:: lz4.frame
.. default-role:: obj

Quickstart
==========

Simple usage
------------

The recommended binding to use is the LZ4 frame format binding, since this
provides interoperability with other implementations and language bindings.

The simplest way to use the frame bindings is via the :py:func:`compress` and
:py:func:`decompress` functions:

.. doctest::

  >>> import os
  >>> import lz4.frame
  >>> input_data = 20 * 128 * os.urandom(1024)  # Read 20 * 128kb
  >>> compressed = lz4.frame.compress(input_data)
  >>> decompressed = lz4.frame.decompress(compressed)
  >>> decompressed == input_data
  True

The :py:func:`compress` function reads the input data and compresses it and
returns a LZ4 frame. A frame consists of a header, and a sequence of blocks of
compressed data, and a frame end marker (and optionally a checksum of the
uncompressed data). The :py:func:`decompress` function takes a full LZ4 frame,
decompresses it (and optionally verifies the uncompressed data against the
stored checksum), and returns the uncompressed data.

Working with data in chunks
---------------------------

It's often inconvenient to hold the full data in memory, and so functions are
also provided to compress and decompress data in chunks:

.. doctest::

  >>> import lz4.frame
  >>> import os
  >>> input_data = 20 * 128 * os.urandom(1024)
  >>> c_context = lz4.frame.create_compression_context()
  >>> compressed = lz4.frame.compress_begin(c_context)
  >>> compressed += lz4.frame.compress_chunk(c_context, input_data[:10 * 128 * 1024])
  >>> compressed += lz4.frame.compress_chunk(c_context, input_data[10 * 128 * 1024:])
  >>> compressed += lz4.frame.compress_flush(c_context)

Here a compression context is first created which is used to maintain state
across calls to the LZ4 library. This is an opaque PyCapsule object.
:py:func:`compress_begin` starts a new frame and returns the frame header.
:py:func:`compress_chunk` compresses input data and returns the compressed data.
:py:func:`compress_flush` ends the frame and returns the frame end marker. The
data returned from these functions is catenated to form the compressed frame.

:py:func:`compress_flush` also flushes any buffered data; by default,
:py:func:`compress_chunk` may buffer data until a block is full. This buffering
can be disabled by specifying ``auto_flush=True`` when calling
:py:func:`compress_begin`. Alternatively, the LZ4 buffers can be flushed at any
time without ending the frame by calling :py:func:`compress_flush` with
``end_frame=False``.

Decompressing data can also be done in a chunked fashion:

.. doctest::

  >>> d_context = lz4.frame.create_decompression_context()
  >>> d1, b, e = lz4.frame.decompress_chunk(d_context, compressed[:len(compressed)//2])
  >>> d2, b, e = lz4.frame.decompress_chunk(d_context, compressed[len(compressed)//2:])
  >>> d1 + d2 == input_data
  True

Note that :py:func:`decompress_chunk` returns a tuple ``(decompressed_data,
bytes_read, end_of_frame_indicator)``. ``decompressed_data`` is the decompressed
data, ``bytes_read`` reports the number of bytes read from the compressed input.
``end_of_frame_indicator`` is ``True`` if the end-of-frame marker is encountered
during the decompression, and ``False`` otherwise. If the end-of-frame marker is
encountered in the input, no attempt is made to decompress the data after the
marker.

Rather than managing compression and decompression context objects manually, it
is more convenient to use the :py:class:`LZ4FrameCompressor` and
:py:class:`LZ4FrameDecompressor` classes which provide context manager
functionality:

.. doctest::

  >>> import lz4.frame
  >>> import os
  >>> input_data = 20 * 128 * os.urandom(1024)
  >>> with lz4.frame.LZ4FrameCompressor() as compressor:
  ...     compressed = compressor.begin()
  ...     compressed += compressor.compress(input_data[:10 * 128 * 1024])
  ...     compressed += compressor.compress(input_data[10 * 128 * 1024:])
  ...     compressed += compressor.flush()
  >>> with lz4.frame.LZ4FrameDecompressor() as decompressor:
  ...     decompressed = decompressor.decompress(compressed[:len(compressed)//2])
  ...     decompressed += decompressor.decompress(compressed[len(compressed)//2:])
  >>> decompressed == input_data
  True


Working with compressed files
-----------------------------

The frame bindings provide capability for working with files containing LZ4
frame compressed data. This functionality is intended to be a drop in
replacement for that offered in the Python standard library for bz2, gzip and
LZMA compressed files. The :py:func:`lz4.frame.open()` function is the most
convenient way to work with compressed data files:

.. doctest::

  >>> import lz4.frame
  >>> import os
  >>> input_data = 20 * os.urandom(1024)
  >>> with lz4.frame.open('testfile', mode='wb') as fp:
  ...     bytes_written = fp.write(input_data)
  ...     bytes_written == len(input_data)
  True
  >>> with lz4.frame.open('testfile', mode='r') as fp:
  ...     output_data = fp.read()
  >>> output_data == input_data
  True

The library also provides the class :py:class:`lz4.frame.LZ4FrameFile` for
working with compressed files.


Controlling the compression
---------------------------

Beyond the basic usage described above, there are a number of keyword arguments
to tune and control the compression. A few of the key ones are listed below,
please see the documentation for full details of options.


Controlling the compression level
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``compression_level`` argument specifies the level of compression used with
0 (default) being the lowest compression (0-2 are the same value), and 16 the
highest compression. Values below 0 will enable "fast acceleration",
proportional to the value. Values above 16 will be treated as 16. The following
module constants are provided as a convenience:

- `lz4.frame.COMPRESSIONLEVEL_MIN`: Minimum compression (0, default)
- `lz4.frame.COMPRESSIONLEVEL_MINHC`: Minimum high-compression mode (3)
- `lz4.frame.COMPRESSIONLEVEL_MAX`: Maximum compression (16)

Availability: :py:func:`lz4.frame.compress()`,
:py:func:`lz4.frame.compress_begin()`, :py:func:`lz4.frame.open()`,
:py:class:`lz4.frame.LZ4FrameCompressor`, :py:class:`lz4.frame.LZ4FrameFile`.


Controlling the block size
~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``block_size`` argument specifies the maximum block size to use for the
blocks in a frame. Options:

- `lz4.frame.BLOCKSIZE_DEFAULT` or 0: the lz4 library default
- `lz4.frame.BLOCKSIZE_MAX64KB` or 4: 64 kB
- `lz4.frame.BLOCKSIZE_MAX256KB` or 5: 256 kB
- `lz4.frame.BLOCKSIZE_MAX1MB` or 6: 1 MB
- `lz4.frame.BLOCKSIZE_MAX4MB` or 7: 4 MB

If unspecified, will default to `lz4.frame.BLOCKSIZE_DEFAULT` which is
currently equal to `lz4.frame.BLOCKSIZE_MAX64KB`

Availability: :py:func:`lz4.frame.compress()`,
:py:func:`lz4.frame.compress_begin()`, :py:func:`lz4.frame.open()`,
:py:class:`lz4.frame.LZ4FrameCompressor`, :py:class:`lz4.frame.LZ4FrameFile`.


Controlling block linking
~~~~~~~~~~~~~~~~~~~~~~~~~

The ``block_linked`` argument specifies whether to use block-linked compression.
If ``True``, the compression process will use data between sequential blocks to
improve the compression ratio, particularly for small blocks. The default is
``True``.

Availability: :py:func:`lz4.frame.compress()`,
:py:func:`lz4.frame.compress_begin()`, :py:func:`lz4.frame.open()`,
:py:class:`lz4.frame.LZ4FrameCompressor`, :py:class:`lz4.frame.LZ4FrameFile`.


Data checksum validation
~~~~~~~~~~~~~~~~~~~~~~~~

The ``content_checksum`` argument specifies whether to enable checksumming of
the uncompressed content. If ``True``, a checksum of the uncompressed data is
stored at the end of the frame, and checked during decompression. Default is
``False``.

The ``block_checksum`` argument specifies whether to enable checksumming of the
uncompressed content of each individual block in the frame. If ``True``, a
checksum is stored at the end of each block in the frame, and checked during
decompression. Default is ``False``.

Availability: :py:func:`lz4.frame.compress()`,
:py:func:`lz4.frame.compress_begin()`, :py:func:`lz4.frame.open()`,
:py:class:`lz4.frame.LZ4FrameCompressor`, :py:class:`lz4.frame.LZ4FrameFile`.


Data buffering
~~~~~~~~~~~~~~

The LZ4 library can be set to buffer data internally until a block is filed in
order to optimize compression. The ``auto_flush`` argument specifies whether the
library should buffer input data or not.

When ``auto_flush`` is ``False`` the LZ4 library may buffer data internally. In
this case, the compression functions may return no compressed data when called.
This is the default.

When ``auto_flush`` is ``True``, the compression functions will return
compressed data immediately.
 
Availability: :py:func:`lz4.frame.compress()`,
:py:func:`lz4.frame.compress_begin()`, :py:func:`lz4.frame.open()`,
:py:class:`lz4.frame.LZ4FrameCompressor`, :py:class:`lz4.frame.LZ4FrameFile`.


Storing the uncompressed source data size in the frame
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``store_size`` and ``source_size`` arguments allow for storing the size of
the uncompressed data in the frame header. Storing the source size in the frame
header adds an extra 8 bytes to the size of the compressed frame, but allows the
decompression functions to better size memory buffers during decompression.

If ``store_size`` is ``True`` the size of the uncompressed data will be stored in
the frame header. Default is ``True``.

Availability of ``store_size``: :py:func:`lz4.frame.compress()`

The ``source_size`` argument optionally specifies the uncompressed size of the
source data to be compressed. If specified, the size will be stored in the frame
header.

Availability of ``source_size``: :py:meth:`lz4.frame.LZ4FrameCompressor.begin()`,
:py:func:`lz4.frame.compress_begin()`, :py:func:`lz4.frame.open()`,
:py:class:`lz4.frame.LZ4FrameFile`.

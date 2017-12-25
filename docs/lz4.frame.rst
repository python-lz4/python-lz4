.. _lz4.frame:

lz4.frame sub-package
=====================

This sub-package is in beta testing. Ahead of version 1.0 there may be API
changes, but these are expected to be minimal, if any.

This sub-package provides the capability to compress and decompress data using
the `LZ4 frame specification <http://lz4.github.io/lz4/lz4_Frame_format.html>`_.

The frame specification is recommended for most applications. A key benefit of
using the frame specification (compared to the block specification) is
interoperability with other implementations.


Contents
--------

.. automodule:: lz4.frame
   :members:
      decompress,
      compress,
      create_compression_context,
      compress_begin,
      compress_chunk,
      compress_flush,
      get_frame_info,
      create_decompression_context,
      decompress_chunk,
      LZ4FrameCompressor,
      LZ4FrameDecompressor


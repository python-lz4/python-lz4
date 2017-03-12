.. _lz4.frame:

lz4.frame sub-package
=====================

This sub-package is an incomplete technology preview.

This sub-package provides the capability to compress and decompress data using
the `LZ4 frame specification <http://lz4.github.io/lz4/lz4_Frame_format.html>`_.

The frame specification is recommended for most applications. A key benefit of
using the frame specification (compared to the block specification) is
interoperability with other implementations.


Contents
--------

.. automodule:: lz4.frame
   :members:
      compress, decompress,
      create_compression_context,
      compress_begin, compress_update, compress_end,
      get_frame_info


lz4.frame package
=================

This module provides the capability to compress and decompress data using the
LZ4 frame specification. A key benefit of using the frame specification
(compared to the block specification) is interoperability with other
implementations.

The LZ4 frame specification can be found on the author's page LZ4 frame
specification_.

.. _specification: https://github.com/Cyan4973/lz4/wiki/lz4_Frame_format.md>`_

Module contents
---------------

.. automodule:: lz4.frame
   :members:
      create_compression_context, free_compression_context,
      compress_frame, compress_begin, compress_update, compress_end,
      create_decompression_context, free_decompression_context,
      decompress_frame, get_frame_info
    :undoc-members:
    :show-inheritance:

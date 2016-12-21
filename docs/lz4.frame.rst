lz4.frame package
=================

This module provides the capability to compress and decompress data using the
`LZ4 frame specification`_.

The frame specification is recommended for most applications. A key benefit of
using the frame specification (compared to the block specification) is
interoperability with other implementations.

.. _`LZ4 frame specification`: http://lz4.github.io/lz4/lz4_Frame_format.html

Module contents
---------------

.. automodule:: lz4.frame
   :members:
      compress, decompress,
      create_compression_context, free_compression_context,
      compress_begin, compress_update, compress_end,
      get_frame_info
    :undoc-members:
    :show-inheritance:

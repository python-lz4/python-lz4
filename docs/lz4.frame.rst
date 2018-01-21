.. py:module:: lz4.frame

lz4.frame sub-package
=====================

This sub-package is in beta testing. Ahead of version 1.0 there may be API
changes, but these are expected to be minimal, if any.

This sub-package provides the capability to compress and decompress data using
the `LZ4 frame specification <http://lz4.github.io/lz4/lz4_Frame_format.html>`_.

The frame specification is recommended for most applications. A key benefit of
using the frame specification (compared to the block specification) is
interoperability with other implementations.


Low level bindings for full content (de)compression
---------------------------------------------------

These functions are bindings to the LZ4 Frame API functions for compressing data
into a single frame, and decompressing a full frame of data.

.. autofunction:: lz4.frame.compress
.. autofunction:: lz4.frame.decompress


Low level bindings for chunked content (de)compression
------------------------------------------------------

These functions are bindings to the LZ4 Frame API functions allowing piece-wise
compression and decompression. Using them requires managing compression and
decompression contexts manually. An alternative to using these is to use the
context manager classes described in the section below.


Compression
~~~~~~~~~~~

.. autofunction:: lz4.frame.create_compression_context
.. autofunction:: lz4.frame.compress_begin
.. autofunction:: lz4.frame.compress_chunk
.. autofunction:: lz4.frame.compress_flush


Decompression
~~~~~~~~~~~~~

.. autofunction:: lz4.frame.create_decompression_context
.. autofunction:: lz4.frame.reset_decompression_context
.. autofunction:: lz4.frame.decompress_chunk


Retrieving frame information
----------------------------

The following function can be used to retrieve information about a compressed frame.

.. autofunction:: lz4.frame.get_frame_info


Helper context manager classes
------------------------------

These classes, which utilize the low level bindings to the Frame API are more
convenient to use. They provide context management, and so it is not necessary
to manually create and manage compression and decompression contexts.

.. autoclass:: lz4.frame.LZ4FrameCompressor
.. autoclass:: lz4.frame.LZ4FrameDecompressor


Reading and writing compressed files
------------------------------------

These provide capability for reading and writing of files using LZ4 compressed
frames. These are designed to be drop in replacements for the LZMA, BZ2 and Gzip
equivalent functionalities in the Python standard library.

.. autofunction:: lz4.frame.open
.. autoclass:: lz4.frame.LZ4FrameFile

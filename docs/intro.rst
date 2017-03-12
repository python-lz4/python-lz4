Introduction
============

This package provides a Python interface for the `LZ4 compression library
<http://lz4.github.io/lz4/>`_ by Yann Collet. Support is provided for Python 2
(from 2.6 onwards) and Python 3 (from 3.3 onwards).

The LZ4 library provides support for three specifications:

* The `frame <http://lz4.github.io/lz4/lz4_Frame_format.html>`_ format
* The `block <http://lz4.github.io/lz4/lz4_Block_format.html>`_ format
* The `stream <https://github.com/lz4/lz4/wiki/LZ4-Streaming-API-Basics>`_ format

This Python interface currently supports the block format. Support for the frame
format is also included as an unstable technology preview. Support for the
streaming format will be available in a future release.

..
   For most applications, the frame format is what you should use as this
   guarantees interoperability with other bindings.

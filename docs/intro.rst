Introduction
============

This package provides a Python interface for the `LZ4 compression library
<http://lz4.github.io/lz4/>`_ by Yann Collet. Support is provided for Python 2
(from 2.7 onwards) and Python 3 (from 3.4 onwards).

The LZ4 library provides support for three specifications:

* The `frame <http://lz4.github.io/lz4/lz4_Frame_format.html>`_ format
* The `block <http://lz4.github.io/lz4/lz4_Block_format.html>`_ format
* The `stream <https://github.com/lz4/lz4/wiki/LZ4-Streaming-API-Basics>`_ format

This Python interface currently supports the frame and block formats. Support
for the streaming format will be available in a future release.

For most applications, the frame format is what you should use as this
guarantees interoperability with other bindings. The frame format defines a
standard container for the compressed data. In the frame format, the data is
compressed into a sequence of blocks. The frame format defines a frame header,
which contains information about the compressed data such as its size, and
defines a standard end of frame marker.

The API provided by the frame format bindings follows that of the LZMA, zlib,
gzip and bzip2 compression libraries which are provided with the Python standard
library. As such, these LZ4 bindings should provide a drop-in alternative to the
compression libraries shipped with Python. The package provides context managers
and file handler support.

The bindings drop the GIL when calling in to the underlying LZ4 library, and is
thread safe. An extensive test suite is included.

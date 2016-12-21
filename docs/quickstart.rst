Quickstart
==========

The recommended binding to use is the LZ4 frame format. The simplest way to use
the frame package is to import the compress and decompress functions::

    >>> import os
    >>> from lz4.frame import compress, decompress
    >>> input_data = os.urandom(20 * 128 * 1024)  # Read 20 * 128kb
    >>> compressed = compress(input_data)
    >>> decompressed = decompress(compressed)
    >>> decompressed == input_data
    Out[6]: True

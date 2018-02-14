.. default-role:: obj


lz4.block sub-package
=====================

This sub-package provides the capability to compress and decompress data using
the `block specification <http://lz4.github.io/lz4/lz4_Block_format.html>`_.

Because the LZ4 block format doesn't define a container format, the Python
bindings will by default insert the original data size as an integer at the
start of the compressed payload, like most other bindings do (Java...). However,
it is possible to disable this functionality.



Example usage
-------------
To use the lz4 block format bindings is straightforward::

.. doctest::

    >>> import lz4.block
    >>> import os
    >>> input_data = 20 * 128 * os.urandom(1024)  # Read 20 * 128kb
    >>> compressed_data = lz4.block.compress(input_data)
    >>> output_data = lz4.block.decompress(compressed_data)
    >>> input_data == output_data
    True


Contents
----------------

.. automodule:: lz4.block
    :members: compress, decompress


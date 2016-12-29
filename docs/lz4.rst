lz4 package
===========

Most of the functionality of this package is found in the :ref:`lz4.frame` and
the :ref:`lz4.block` format bindings. The base package functionality is
described here.

Contents
--------

.. automodule:: lz4
   :members: lz4version
   :undoc-members:
   :show-inheritance:

Deprecated functions
~~~~~~~~~~~~~~~~~~~~

The following methods are provided as wrappers around ``compress`` and
``decompress`` for backwards compatibility, but will be removed in the
near future. These methods are also imported into the lz4 top level
namespace for backwards compatibility.

This documentation is intentionally vague so as to discourage you from using
these functions. Code that uses the functions will raised a
``DeprecatedWarning``.

- ``dumps``, and ``LZ4_compress`` are wrappers around ``lz4.block.compress``
- ``loads``, ``uncompress``, ``LZ4_uncompress`` are wrappers around
  ``lz4.block.decompress``
- ``compress_fast`` and ``LZ4_compress_fast`` are wrappers around
  ``lz4.block.compress`` with ``mode=fast``
- ``compressHC`` is a wrapper around ``lz4.block.compress`` with
  ``mode=high_compression``



import lz4
from ._frame import *
from ._frame import __doc__ as _doc
__doc__ = _doc


class LZ4FrameCompressor(object):
    """Create a LZ4 compressor object, which can be used to compress data
    incrementally.

    Args:
        block_size (int): Sepcifies the maximum blocksize to use.
            Options:

                - lz4.frame.BLOCKSIZE_DEFAULT or 0: the lz4 library default
                - lz4.frame.BLOCKSIZE_MAX64KB or 4: 64 kB
                - lz4.frame.BLOCKSIZE_MAX256KB or 5: 256 kB
                - lz4.frame.BLOCKSIZE_MAX1MB or 6: 1 MB
                - lz4.frame.BLOCKSIZE_MAX4MB or 7: 4 MB

            If unspecified, will default to lz4.frame.BLOCKSIZE_DEFAULT which
            is equal to lz4.frame.BLOCKSIZE_MAX64KB.
        block_linked (bool): Specifies whether to use block-linked
            compression. If `True`, the compression ratio is improved, especially
            for small block sizes. If `False` the blocks are compressed independently.
            The default is `True`.
        compression_level (int): Specifies the level of compression used.
            Values between 0-16 are valid, with 0 (default) being the
            lowest compression (0-2 are the same value), and 16 the highest.
            Values above 16 will be treated as 16.
            Values between 4-9 are recommended. 0 is the default.
            The following module constants are provided as a convenience:

                - lz4.frame.COMPRESSIONLEVEL_MIN: Minimum compression (0)
                - lz4.frame.COMPRESSIONLEVEL_MINHC: Minimum high-compression (3)
                - lz4.frame.COMPRESSIONLEVEL_MAX: Maximum compression (16)

        content_checksum (bool): Specifies whether to enable checksumming of
            the payload content. If `True` a checksum of the uncompressed data
            is stored at the end of the compressed frame which is checked during
            decompression. The default is `False`.
        block_checksum (bool): Specifies whether to enable checksumming of
            the content of each block. If `True` a checksum of the uncompressed
            data in each block in the frame is stored at the end of each block.
            If present, these checksums will be used to validate the data during
            decompression. The default is `False`, meaning block checksums are not
            calculated and stored. This functionality is only supported if the
            underlying LZ4 library has version >= 1.8.0. Attempting to set this
            value to `True` with a version of LZ4 < 1.8.0 will cause a RuntimeError
            to be raised.
        auto_flush (bool): When `False`, the LZ4 library may buffer data until a
            block is full. When `True` no buffering occurs, and partially full
            blocks may be returned. The default is `False`.
        return_bytearray (bool): When `False` a bytes object is returned from the
            calls to methods of this class. When `True` a bytearray object will be
            returned. The default is `False`.
    """

    def __init__(self,
                 block_size=BLOCKSIZE_DEFAULT,
                 block_linked=True,
                 compression_level=COMPRESSIONLEVEL_MIN,
                 content_checksum=False,
                 block_checksum=False,
                 auto_flush=False,
                 return_bytearray=False):
        self.block_size = block_size
        self.block_linked = block_linked
        self.compression_level = compression_level
        self.content_checksum = content_checksum
        if block_checksum and lz4.library_version_number() < 10800:
            raise RuntimeError(
                'Attempt to set block_checksum to True with LZ4 library version < 10800'
            )
        self.block_checksum = block_checksum
        self.auto_flush = auto_flush
        self.return_bytearray = return_bytearray
        self._context = create_compression_context()
        self._started = False

    def __enter__(self):
        # All necessary initialization is done in __init__
        return self

    def __exit__(self, exception_type, exception, traceback):
        # The compression context is created with an appropriate destructor, so
        # no need to del it here
        pass

    def begin(self, source_size=0):
        """Begin a compression frame. The returned data contains frame header
        information. The data returned from subsequent calls to ``compress()``
        should be concatenated with this header.

        Args:
            source_size (int): Optionally specify the total size of the
                uncompressed data. If specified, will be stored in the
                compressed frame header as an 8-byte field for later use
                during decompression. Default is 0 (no size stored).

        Returns:
            bytes or bytearray: frame header data
        """

        if self._started is False:
            result = compress_begin(
                self._context,
                block_size=self.block_size,
                block_linked=self.block_linked,
                compression_level=self.compression_level,
                content_checksum=self.content_checksum,
                block_checksum=self.block_checksum,
                auto_flush=self.auto_flush,
                return_bytearray=self.return_bytearray,
                source_size=source_size
            )
            self._started = True
            return result
        else:
            raise RuntimeError('compress_begin called when not already initialized')

    def compress(self, data):
        """Compress ``data`` (a ``bytes`` object), returning a bytes object
        containing compressed data the input.

        If ``auto_flush`` has been set to ``False``, some of ``data`` may be
        buffered internally, for use in later calls to compress() and flush().

        The returned data should be concatenated with the output of any
        previous calls to ``compress()`` and a single call to
        ``compress_begin()``.

        Args:
            data (str, bytes or buffer-compatible object): data to compress

        Returns:
            bytes or bytearray: compressed data

        """
        if self._context is None:
            raise RuntimeError('compress called after flush()')

        if self._started is False:
            raise RuntimeError('compress called before compress_begin()')

        result = compress_chunk(
            self._context, data,
            return_bytearray=self.return_bytearray
        )

        return result

    def finalize(self):
        """Finish the compression process, returning a bytes object containing any data
        stored in the compressor's internal buffers and a frame footer.

        To use the LZ4FrameCompressor instance after this has been called, it
        is necessary to first call the ``reset()`` method.

        Returns:
            bytes or bytearray: any remaining buffered compressed data and frame footer.

        """
        result = compress_flush(
            self._context,
            end_frame=True,
            return_bytearray=self.return_bytearray
        )
        self._context = None
        self._started = False
        return result

    def reset(self):
        """Reset the LZ4FrameCompressor instance allowing it to be re-used

        """
        self._context = create_compression_context()
        self._started = False


class LZ4FrameDecompressor(object):
    """Create a LZ4 frame decompressor object, which can be used to decompress data
    incrementally.

    Args:
        return_bytearray (bool): When ``False`` a bytes object is returned from the
            calls to methods of this class. When ``True`` a bytearray object will be
            returned. The default is ``False``.
        return_bytes_read (bool): When ``True``, calls to ``decompress`` will return
            the number of bytes read from the input data as well as the uncompressed
            data. The default is ``False``.

    """

    def __init__(self, return_bytearray=False, return_bytes_read=False):
        self.return_bytearray = return_bytearray
        self.return_bytes_read = return_bytes_read
        self._context = create_decompression_context()

    def __enter__(self):
        # All necessary initialization is done in __init__
        return self

    def __exit__(self, exception_type, exception, traceback):
        # The decompression context is created with an appropriate destructor,
        # so no need to del it here
        pass

    def decompress(self, data, full_frame=False):
        """Decompresses part of an LZ4 frame of compressed data. The returned data
        should be concatenated with the output of any previous calls to
        ``decompress()``.

        Args:
            data (str, bytes or buffer-compatible object): data to decompress
            full_frame (bool): If ``True``, then the ``data`` argument is expected
                to contain the full LZ4 frame. Default is ``False``.

        Returns:
             bytes/bytearray or tuple: Uncompressed data

             If the ``return_bytes_read`` argument is ``True`` this function
             returns a tuple consisting of:

                 - bytes or bytearray: Uncompressed data
                 - int: Number of bytes consumed from ``data``

        """

        if full_frame is True:
            return decompress(
                self._context,
                data,
                return_bytearray=self.return_bytearray,
                return_bytes_read=self.return_bytes_read,
            )
        else:
            return decompress_chunk(
                self._context,
                data,
                return_bytearray=self.return_bytearray,
                return_bytes_read=self.return_bytes_read,
            )

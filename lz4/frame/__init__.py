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
            If unspecified, will default to lz4.frame.BLOCKSIZE_DEFAULT.
        block_mode (int): Specifies whether to use block-linked
            compression. Options:
            - lz4.frame.BLOCKMODE_LINKED or 0: linked mode
            - lz4.frame.BLOCKMODE_INDEPENDENT or 1: disable linked mode
            The default is lz4.frame.BLOCKMODE_LINKED.
        compression_level (int): Specifies the level of compression used.
            Values between 0-16 are valid, with 0 (default) being the
            lowest compression (0-2 are the same value), and 16 the highest.
            Values above 16 will be treated as 16.
            Values between 4-9 are recommended. 0 is the default.
            The following module constants are provided as a convenience:
            - lz4.frame.COMPRESSIONLEVEL_MIN: Minimum compression (0)
            - lz4.frame.COMPRESSIONLEVEL_MINHC: Minimum high-compression (3)
            - lz4.frame.COMPRESSIONLEVEL_MAX: Maximum compression (16)
        content_checksum (int): Specifies whether to enable checksumming of
            the payload content. Options:
            - lz4.frame.CONTENTCHECKSUM_DISABLED or 0: disables checksumming
            - lz4.frame.CONTENTCHECKSUM_ENABLED or 1: enables checksumming
            The default is CONTENTCHECKSUM_DISABLED.
        frame_type (int): Specifies whether user data can be injected between
            frames. Options:
            - lz4.frame.FRAMETYPE_FRAME or 0: disables user data injection
            - lz4.frame.FRAMETYPE_SKIPPABLEFRAME or 1: enables user data
              injection
        auto_flush (bool): When False, the LZ4 library may buffer data until a
            block is full. When True no buffering occurs, and partially full
            blocks may be returned. The default is True.
    """

    def __init__(self,
                 block_size=BLOCKSIZE_DEFAULT,
                 block_mode=BLOCKMODE_LINKED,
                 compression_level=COMPRESSIONLEVEL_MIN,
                 content_checksum=CONTENTCHECKSUM_DISABLED,
                 frame_type=FRAMETYPE_FRAME,
                 auto_flush=True):
        self.block_size = block_size
        self.block_mode = block_mode
        self.compression_level = compression_level
        self.content_checksum = content_checksum
        self.frame_type = frame_type
        self.auto_flush = auto_flush
        self._context = create_compression_context()
        self._started = False

    def __enter__(self):
        # All necessary initialization is done in __init__
        return self

    def __exit__(self, exception_type, exception, traceback):
        # The compression context is created with an appropriate destructor, so
        # no need to del it here
        pass

    def compress_begin(self, source_size=0):
        """Begin a compression frame. The returned data contains frame header
        information. The data returned from subsequent calls to ``compress()``
        should be concatenated with this header.

        Args:
            data (bytes): data to compress
            source_size (int): Optionally specified the total size of the
                uncompressed data. If specified, will be stored in the
                compressed frame header for later use in decompression.

        Returns:
            bytes: frame header data
        """

        if self._started is False:
            result = compress_begin(self._context,
                                    block_size=self.block_size,
                                    block_mode=self.block_mode,
                                    frame_type=self.frame_type,
                                    compression_level=self.compression_level,
                                    content_checksum=self.content_checksum,
                                    auto_flush=self.auto_flush,
                                    source_size=source_size)

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
            data (bytes): data to compress

        Returns:
            bytes: compressed data

        """
        if self._context is None:
            raise RuntimeError('compress called after flush()')

        if self._started is False:
            raise RuntimeError('compress called before compress_begin()')

        result = compress_update(self._context, data)

        return result

    def flush(self):
        """Finish the compression process, returning a bytes object containing any data
        stored in the compressor's internal buffers and a frame footer.

        To use the LZ4FrameCompressor instance after this has been called, it
        is necessary to first call the ``reset()`` method.

        Returns:
            bytes: any remaining buffered compressed data and frame footer.

        """
        result = compress_end(self._context)
        self._context = None
        self._started = False
        return result

    def reset(self):
        """Reset the LZ4FrameCompressor instance (after a call to ``flush``) allowing
        it to be re-used

        """
        self._context = create_compression_context()
        self._started = False

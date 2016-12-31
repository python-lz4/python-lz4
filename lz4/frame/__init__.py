from ._frame import *
from ._frame import __doc__ as _doc
__doc__ = _doc


class LZ4Compressor(object):

    def __init__(self,
                 block_size=BLOCKSIZE_DEFAULT,
                 block_mode=BLOCKMODE_INDEPENDENT,
                 compression_level=COMPRESSIONLEVEL_MIN,
                 content_checksum=CONTENTCHECKSUM_DISABLED,
                 frame_type=FRAMETYPE_FRAME,
                 auto_flush=1
    ):
        self.block_size = block_size
        self.block_mode = block_mode
        self.compression_level = compression_level
        self.content_checksum = content_checksum
        self.frame_type = frame_type
        self.auto_flush = auto_flush
        self.context = create_compression_context()
        self._started = False

    def __enter__(self):
        # All necessary initialization is done in __init__
        return self

    def __exit__(self, exception_type, exception, traceback):
        # The compression context is created with an appropriate destructor, so
        # no need to del it here
        pass

    def compress(self, data):
        if self._started is False:
            result = compress_begin(self.context,
                                    block_size=self.block_size,
                                    block_mode=self.block_mode,
                                    frame_type=self.frame_type,
                                    compression_level=self.compression_level,
                                    content_checksum=self.content_checksum,
                                    auto_flush=self.auto_flush
            )
            self._started = True
        elif self._started is True:
            result = bytes()
        else:
            raise RuntimeError(
                'Indeterminate state of {0}'.format(self.__class__.__name__)
            )

        result += compress_update(self.context, data)

        return result

    def flush(self):
        return compress_end(self.context)

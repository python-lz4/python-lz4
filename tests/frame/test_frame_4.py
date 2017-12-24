from . helpers import (
    roundtrip_LZ4FrameCompressor,
    roundtrip_LZ4FrameCompressor_LZ4FrameDecompressor,
)
import os
import pytest

test_data=[
    (128 * (32 * os.urandom(32))),
    (256 * (32 * os.urandom(32))),
    (512 * (32 * os.urandom(32))),
    (1024 * (32 * os.urandom(32))),
]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param

@pytest.fixture(
    params=[
        (True),
        (False)
    ]
)
def reset(request):
    return request.param

@pytest.fixture(
    params=[
        (1),
        (8)
    ]
)
def chunks(request):
    return request.param


def test_roundtrip_LZ4FrameCompressor(data, chunks, block_size, reset, block_checksum, content_checksum):
    roundtrip_LZ4FrameCompressor(
        data,
        chunks=chunks,
        block_size=block_size,
        reset=reset,
        block_checksum=block_checksum,
        content_checksum=content_checksum,
    )

def test_roundtrip_LZ4FrameCompressor_LZ4FrameDecompressor(
        data, chunks, block_size, reset, block_checksum, content_checksum):
    roundtrip_LZ4FrameCompressor_LZ4FrameDecompressor(
        data,
        chunks=chunks,
        block_size=block_size,
        reset=reset,
        block_checksum=block_checksum,
        content_checksum=content_checksum,
    )



# class TestLZ4FrameModern(unittest.TestCase):
#     def test_decompress_truncated(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         for chksum in (lz4frame.CONTENTCHECKSUM_DISABLED, lz4frame.CONTENTCHECKSUM_ENABLED):
#             for conlen in (0, len(input_data)):
#                 context = lz4frame.create_compression_context()
#                 compressed = lz4frame.compress_begin(context, content_checksum=chksum, source_size=conlen)
#                 compressed += lz4frame.compress_update(context, input_data)
#                 compressed += lz4frame.compress_end(context)
#                 for i in range(len(compressed)):
#                     with self.assertRaisesRegexp(RuntimeError, r'^(LZ4F_getFrameInfo failed with code: ERROR_frameHeader_incomplete|LZ4F_freeDecompressionContext reported unclean decompressor state \(truncated frame\?\): \d+)$'):
#                         lz4frame.decompress(compressed[:i])

#     def test_checksum_failure(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         compressed = lz4frame.compress(input_data, content_checksum=lz4frame.CONTENTCHECKSUM_ENABLED)
#         with self.assertRaisesRegexp(RuntimeError, r'^LZ4F_decompress failed with code: ERROR_contentChecksum_invalid'):
#             last = struct.unpack('B', compressed[-1:])[0]
#             lz4frame.decompress(compressed[:-1] + struct.pack('B', last ^ 0x42))
#         # NB: blockChecksumFlag is not supported by lz4 at the moment, so some
#         # random 1-bit modifications of input may actually trigger valid output
#         # without errors. And content checksum remains the same!

#     def test_decompress_trailer(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         compressed = lz4frame.compress(input_data)
#         with self.assertRaisesRegexp(ValueError, r'^Extra data: 64 trailing bytes'):
#             lz4frame.decompress(compressed + b'A'*64)
#         # This API does not support frame concatenation!
#         with self.assertRaisesRegexp(ValueError, r'^Extra data: \d+ trailing bytes'):
#             lz4frame.decompress(compressed + compressed)

#     def test_LZ4FrameCompressor_fails(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         with self.assertRaisesRegexp(RuntimeError, r'compress called after flush'):
#             with lz4frame.LZ4FrameCompressor() as compressor:
#                 compressed = compressor.compress_begin()
#                 compressed += compressor.compress(input_data)
#                 compressed += compressor.flush()
#                 compressed = compressor.compress(input_data)


# if sys.version_info < (2, 7):
#     # Poor-man unittest.TestCase.skip for Python 2.6
#     del TestLZ4FrameModern

# if __name__ == '__main__':
#     unittest.main()

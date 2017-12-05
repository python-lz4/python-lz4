import lz4.frame as lz4frame
import pytest
import os
import sys
import struct
from multiprocessing.pool import ThreadPool
import math

def get_chunked(data, nchunks):
    size = len(data)
    stride = int(math.ceil(float(size)/nchunks)) # no // on py 2.6
    start = 0
    end = start + stride
    while end < size:
        yield data[start:end]
        start += stride
        end += stride
    yield data[start:]

def test_roundtrip_chunked(data_chunked, block_size, block_mode,
                           content_checksum, frame_type,
                           compression_level, auto_flush):
    data, c_chunks, d_chunks = data_chunked

    c_context = lz4frame.create_compression_context()
    compressed = lz4frame.compress_begin(
        c_context,
        source_size=len(data),
        compression_level=compression_level,
        block_size=block_size,
        content_checksum=content_checksum,
        frame_type=frame_type,
        auto_flush=auto_flush
    )
    data_in = get_chunked(data, c_chunks)
    try:
        while True:
            compressed += lz4frame.compress_update(
                c_context,
                next(data_in)
            )
    except StopIteration:
        pass
    finally:
        del data_in

    compressed += lz4frame.compress_end(c_context)

    d_context = lz4frame.create_decompression_context()
    compressed_in = get_chunked(compressed, d_chunks)
    decompressed = b''
    bytes_read = 0
    try:
        while True:
            d, b = lz4frame.decompress(d_context, next(compressed_in))
            decompressed += d
            bytes_read += b
    except StopIteration:
        pass
    finally:
        del compressed_in

    #assert bytes_read == len(compressed)
    assert decompressed == data


# class TestLZ4Frame(unittest.TestCase):


#     def test_get_frame_info(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         compressed = lz4frame.compress(
#             input_data,
#             lz4frame.COMPRESSIONLEVEL_MAX,
#             lz4frame.BLOCKSIZE_MAX64KB,
#             lz4frame.CONTENTCHECKSUM_DISABLED,
#             lz4frame.BLOCKMODE_INDEPENDENT,
#             lz4frame.FRAMETYPE_FRAME
#         )

#         frame_info = lz4frame.get_frame_info(compressed)

#         self.assertEqual(
#             frame_info,
#             {
#                 "blockSizeID":lz4frame.BLOCKSIZE_MAX64KB,
#                 "blockMode":lz4frame.BLOCKMODE_INDEPENDENT,
#                 "contentChecksumFlag":lz4frame.CONTENTCHECKSUM_DISABLED,
#                 "frameType":lz4frame.FRAMETYPE_FRAME,
#                 "contentSize":len(input_data)
#             }
#         )

#     def test_threads(self):
#         data = [os.urandom(128 * 1024) for i in range(100)]
#         def roundtrip(x):
#             return lz4frame.decompress(lz4frame.compress(x))

#         pool = ThreadPool(8)
#         out = pool.map(roundtrip, data)
#         pool.close()
#         self.assertEqual(data, out)

#     def test_compress_begin_update_end_no_auto_flush_not_defaults_threaded(self):
#         data = [os.urandom(3 * 256 * 1024) for i in range(100)]

#         def roundtrip(x):
#             context = lz4frame.create_compression_context()
#             self.assertNotEqual(context, None)
#             compressed = lz4frame.compress_begin(
#                 context,
#                 block_size=lz4frame.BLOCKSIZE_MAX256KB,
#                 block_mode=lz4frame.BLOCKMODE_LINKED,
#                 compression_level=lz4frame.COMPRESSIONLEVEL_MAX,
#                 auto_flush=0
#             )
#             chunk_size = 128 * 1024 # 128 kb, half of block size
#             start = 0
#             end = start + chunk_size

#             while start <= len(x):
#                 compressed += lz4frame.compress_update(context, x[start:end])
#                 start = end
#                 end = start + chunk_size

#             compressed += lz4frame.compress_end(context)
#             decompressed = lz4frame.decompress(compressed)
#             return decompressed

#         pool = ThreadPool(8)
#         out = pool.map(roundtrip, data)
#         pool.close()
#         self.assertEqual(data, out)

#     def test_LZ4FrameCompressor(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         with lz4frame.LZ4FrameCompressor() as compressor:
#             compressed = compressor.compress_begin()
#             compressed += compressor.compress(input_data)
#             compressed += compressor.flush()
#         decompressed = lz4frame.decompress(compressed)
#         self.assertEqual(input_data, decompressed)

#     def test_LZ4FrameCompressor_reset(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         with lz4frame.LZ4FrameCompressor() as compressor:
#             compressed = compressor.compress_begin()
#             compressed += compressor.compress(input_data)
#             compressed += compressor.flush()
#             compressor.reset()
#             compressed = compressor.compress_begin()
#             compressed += compressor.compress(input_data)
#             compressed += compressor.flush()
#         decompressed = lz4frame.decompress(compressed)
#         self.assertEqual(input_data, decompressed)

#     def test_compress_without_content_size(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         compressed = lz4frame.compress(input_data, content_size_header=False)
#         frame = lz4frame.get_frame_info(compressed)
#         self.assertEqual(frame['contentSize'], 0)
#         decompressed = lz4frame.decompress(compressed)
#         self.assertEqual(input_data, decompressed)

#     def test_LZ4FrameCompressor2(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         with lz4frame.LZ4FrameCompressor() as compressor:
#             compressed = compressor.compress_begin()
#             compressed += compressor.compress(input_data)
#             compressed += compressor.flush()
#         dctx = lz4frame.create_decompression_context()
#         decompressed, read = lz4frame.decompress2(dctx, compressed, full_frame=True)
#         self.assertEqual(input_data, decompressed)
#         self.assertEqual(read, len(compressed))

#     def test_LZ4FrameCompressor2b(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         with lz4frame.LZ4FrameCompressor() as compressor:
#             compressed = compressor.compress_begin()
#             compressed += compressor.compress(input_data)
#             compressed += compressor.flush()
#         mid = int(len(compressed) / 2)
#         dctx = lz4frame.create_decompression_context()
#         decompressed, read = lz4frame.decompress2(dctx, compressed[0:mid])
#         decompressed, r = lz4frame.decompress2(dctx, compressed[read:])
#         read += r
#         self.assertEqual(input_data, decompressed)
#         self.assertEqual(read, len(compressed))

#     def test_LZ4FrameCompressor2b(self):
#         input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
#         with lz4frame.LZ4FrameCompressor() as compressor:
#             compressed = compressor.compress_begin()
#             compressed += compressor.compress(input_data)
#             compressed += compressor.flush()
#         mid = int(len(compressed) / 2)
#         with lz4frame.LZ4FrameDecompressor() as decompressor:
#             decompressed, read = decompressor.decompress(compressed[0:mid])
#             decompressed, r = decompressor.decompress(compressed[read:])
#             read += r
#         self.assertEqual(input_data, decompressed)
#         self.assertEqual(read, len(compressed))


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

import lz4.frame as lz4frame
import unittest
import os
import sys
import struct
from multiprocessing.pool import ThreadPool

class TestLZ4Frame(unittest.TestCase):
    def test_create_and_free_compression_context(self):
        context = lz4frame.create_compression_context()
        self.assertNotEqual(context, None)

    def test_compress(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        compressed = lz4frame.compress(input_data)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_begin_update_end(self):
        context = lz4frame.create_compression_context()
        self.assertNotEqual(context, None)

        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        chunk_size = int((len(input_data)/2)+1)
        compressed = lz4frame.compress_begin(context)
        compressed += lz4frame.compress_update(context, input_data[:chunk_size])
        compressed += lz4frame.compress_update(context, input_data[chunk_size:])
        compressed += lz4frame.compress_end(context)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_huge_with_size(self):
        context = lz4frame.create_compression_context()
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123" * 4096
        chunk_size = int((len(input_data)/2)+1)
        compressed = lz4frame.compress_begin(context, source_size=len(input_data))
        compressed += lz4frame.compress_update(context, input_data[:chunk_size])
        compressed += lz4frame.compress_update(context, input_data[chunk_size:])
        compressed += lz4frame.compress_end(context)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_huge_without_size(self):
        context = lz4frame.create_compression_context()
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123" * 4096
        chunk_size = int((len(input_data)/2)+1)
        compressed = lz4frame.compress_begin(context)
        compressed += lz4frame.compress_update(context, input_data[:chunk_size])
        compressed += lz4frame.compress_update(context, input_data[chunk_size:])
        compressed += lz4frame.compress_end(context)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_not_defaults_1(self):
        input_data = os.urandom(10 * 128 * 1024)  # Read 10 * 128kb
        compressed = lz4frame.compress(
            input_data,
            block_size=lz4frame.BLOCKSIZE_MAX256KB,
            block_mode=lz4frame.BLOCKMODE_LINKED,
            compression_level=lz4frame.COMPRESSIONLEVEL_MAX,
        )
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_not_defaults_2(self):
        input_data = os.urandom(20 * 128 * 1024)  # Read 20 * 128kb
        compressed = lz4frame.compress(
            input_data,
            block_size=lz4frame.BLOCKSIZE_MAX1MB,
            block_mode=lz4frame.BLOCKMODE_INDEPENDENT,
            compression_level=lz4frame.COMPRESSIONLEVEL_MIN,
            content_checksum=lz4frame.CONTENTCHECKSUM_DISABLED
        )
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_not_defaults_3(self):
        input_data = os.urandom(20 * 128 * 1024)  # Read 20 * 128kb
        compressed = lz4frame.compress(
            input_data,
            block_size=lz4frame.BLOCKSIZE_MAX64KB,
            block_mode=lz4frame.BLOCKMODE_LINKED,
            compression_level=lz4frame.COMPRESSIONLEVEL_MINHC,
            content_checksum=lz4frame.CONTENTCHECKSUM_DISABLED
        )
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_not_defaults_4(self):
        input_data = os.urandom(20 * 128 * 1024)  # Read 20 * 128kb
        compressed = lz4frame.compress(
            input_data,
            block_size=lz4frame.BLOCKSIZE_MAX64KB,
            block_mode=lz4frame.BLOCKMODE_LINKED,
            compression_level=lz4frame.COMPRESSIONLEVEL_MAX,
            content_checksum=lz4frame.CONTENTCHECKSUM_ENABLED
        )
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_begin_update_end_no_auto_flush(self):
        context = lz4frame.create_compression_context()
        self.assertNotEqual(context, None)

        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        chunk_size = int((len(input_data)/2)+1)
        compressed = lz4frame.compress_begin(context, auto_flush=0)
        compressed += lz4frame.compress_update(context, input_data[:chunk_size])
        compressed += lz4frame.compress_update(context, input_data[chunk_size:])
        compressed += lz4frame.compress_end(context)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_begin_update_end_no_auto_flush_2(self):
        input_data = os.urandom(4 * 128 * 1024)  # Read 4 * 128kb
        context = lz4frame.create_compression_context()
        self.assertNotEqual(context, None)
        compressed = lz4frame.compress_begin(context, auto_flush=0)
        chunk_size = 32 * 1024 # 32 kb, half of default block size
        start = 0
        end = start + chunk_size

        while start <= len(input_data):
            compressed += lz4frame.compress_update(context, input_data[start:end])
            start = end
            end = start + chunk_size

        compressed += lz4frame.compress_end(context)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_begin_update_end_not_defaults(self):
        input_data = os.urandom(10 * 128 * 1024)  # Read 10 * 128kb
        context = lz4frame.create_compression_context()
        self.assertNotEqual(context, None)
        compressed = lz4frame.compress_begin(
            context,
            block_size=lz4frame.BLOCKSIZE_MAX256KB,
            block_mode=lz4frame.BLOCKMODE_LINKED,
            compression_level=lz4frame.COMPRESSIONLEVEL_MINHC,
            auto_flush=1
        )
        chunk_size = 128 * 1024 # 128 kb, half of block size
        start = 0
        end = start + chunk_size

        while start <= len(input_data):
            compressed += lz4frame.compress_update(context, input_data[start:end])
            start = end
            end = start + chunk_size

        compressed += lz4frame.compress_end(context)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_begin_update_end_no_auto_flush_not_defaults(self):
        input_data = os.urandom(10 * 128 * 1024)  # Read 10 * 128kb
        context = lz4frame.create_compression_context()
        self.assertNotEqual(context, None)
        compressed = lz4frame.compress_begin(
            context,
            block_size=lz4frame.BLOCKSIZE_MAX256KB,
            block_mode=lz4frame.BLOCKMODE_LINKED,
            compression_level=lz4frame.COMPRESSIONLEVEL_MAX,
            auto_flush=0
        )
        chunk_size = 128 * 1024 # 128 kb, half of block size
        start = 0
        end = start + chunk_size

        while start <= len(input_data):
            compressed += lz4frame.compress_update(context, input_data[start:end])
            start = end
            end = start + chunk_size

        compressed += lz4frame.compress_end(context)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_get_frame_info(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        compressed = lz4frame.compress(
            input_data,
            lz4frame.COMPRESSIONLEVEL_MAX,
            lz4frame.BLOCKSIZE_MAX64KB,
            lz4frame.CONTENTCHECKSUM_DISABLED,
            lz4frame.BLOCKMODE_INDEPENDENT,
            lz4frame.FRAMETYPE_FRAME
        )

        frame_info = lz4frame.get_frame_info(compressed)

        self.assertEqual(
            frame_info,
            {
                "blockSizeID":lz4frame.BLOCKSIZE_MAX64KB,
                "blockMode":lz4frame.BLOCKMODE_INDEPENDENT,
                "contentChecksumFlag":lz4frame.CONTENTCHECKSUM_DISABLED,
                "frameType":lz4frame.FRAMETYPE_FRAME,
                "contentSize":len(input_data)
            }
        )

    def test_threads(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtrip(x):
            return lz4frame.decompress(lz4frame.compress(x))

        pool = ThreadPool(8)
        out = pool.map(roundtrip, data)
        pool.close()
        self.assertEqual(data, out)

    def test_compress_begin_update_end_no_auto_flush_not_defaults_threaded(self):
        data = [os.urandom(3 * 256 * 1024) for i in range(100)]

        def roundtrip(x):
            context = lz4frame.create_compression_context()
            self.assertNotEqual(context, None)
            compressed = lz4frame.compress_begin(
                context,
                block_size=lz4frame.BLOCKSIZE_MAX256KB,
                block_mode=lz4frame.BLOCKMODE_LINKED,
                compression_level=lz4frame.COMPRESSIONLEVEL_MAX,
                auto_flush=0
            )
            chunk_size = 128 * 1024 # 128 kb, half of block size
            start = 0
            end = start + chunk_size

            while start <= len(x):
                compressed += lz4frame.compress_update(context, x[start:end])
                start = end
                end = start + chunk_size

            compressed += lz4frame.compress_end(context)
            decompressed = lz4frame.decompress(compressed)
            return decompressed

        pool = ThreadPool(8)
        out = pool.map(roundtrip, data)
        pool.close()
        self.assertEqual(data, out)

    def test_LZ4FrameCompressor(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        with lz4frame.LZ4FrameCompressor() as compressor:
            compressed = compressor.compress_begin()
            compressed += compressor.compress(input_data)
            compressed += compressor.flush()
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_LZ4FrameCompressor_reset(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        with lz4frame.LZ4FrameCompressor() as compressor:
            compressed = compressor.compress_begin()
            compressed += compressor.compress(input_data)
            compressed += compressor.flush()
            compressor.reset()
            compressed = compressor.compress_begin()
            compressed += compressor.compress(input_data)
            compressed += compressor.flush()
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

class TestLZ4FrameModern(unittest.TestCase):
    def test_decompress_truncated(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        for chksum in (lz4frame.CONTENTCHECKSUM_DISABLED, lz4frame.CONTENTCHECKSUM_ENABLED):
            for conlen in (0, len(input_data)):
                context = lz4frame.create_compression_context()
                compressed = lz4frame.compress_begin(context, content_checksum=chksum, source_size=conlen)
                compressed += lz4frame.compress_update(context, input_data)
                compressed += lz4frame.compress_end(context)
                for i in range(len(compressed)):
                    with self.assertRaisesRegexp(RuntimeError, r'^(LZ4F_getFrameInfo failed with code: ERROR_frameHeader_incomplete|LZ4F_freeDecompressionContext reported unclean decompressor state \(truncated frame\?\): \d+)$'):
                        lz4frame.decompress(compressed[:i])

    def test_checksum_failure(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        compressed = lz4frame.compress(input_data, content_checksum=lz4frame.CONTENTCHECKSUM_ENABLED)
        with self.assertRaisesRegexp(RuntimeError, r'^LZ4F_decompress failed with code: ERROR_contentChecksum_invalid'):
            last = struct.unpack('B', compressed[-1:])[0]
            lz4frame.decompress(compressed[:-1] + struct.pack('B', last ^ 0x42))
        # NB: blockChecksumFlag is not supported by lz4 at the moment, so some
        # random 1-bit modifications of input may actually trigger valid output
        # without errors. And content checksum remains the same!

    def test_decompress_trailer(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        compressed = lz4frame.compress(input_data)
        with self.assertRaisesRegexp(ValueError, r'^Extra data: 64 trailing bytes'):
            lz4frame.decompress(compressed + b'A'*64)
        # This API does not support frame concatenation!
        with self.assertRaisesRegexp(ValueError, r'^Extra data: \d+ trailing bytes'):
            lz4frame.decompress(compressed + compressed)

    def test_LZ4FrameCompressor_fails(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        with self.assertRaisesRegexp(RuntimeError, r'compress called after flush'):
            with lz4frame.LZ4FrameCompressor() as compressor:
                compressed = compressor.compress_begin()
                compressed += compressor.compress(input_data)
                compressed += compressor.flush()
                compressed = compressor.compress(input_data)

if sys.version_info < (2, 7):
    # Poor-man unittest.TestCase.skip for Python 2.6
    del TestLZ4FrameModern

if __name__ == '__main__':
    unittest.main()

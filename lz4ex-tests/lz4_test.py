from lz4ex import lz4
import unittest


class TestLZ4(unittest.TestCase):
    def test_compress_default(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        input_data_size = len(input_data)
        compressed = lz4.compress_default(input_data)
        decompressed = lz4.decompress_safe(compressed, input_data_size)
        self.assertEqual(input_data, decompressed)

    def test_compress_fast(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        input_data_size = len(input_data)
        compressed = lz4.compress_fast(input_data, 6)
        decompressed = lz4.decompress_safe(compressed, input_data_size)
        self.assertEqual(input_data, decompressed)

    def test_decompress_invalid(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        input_data_size = len(input_data)
        compressed = lz4.compress_default(input_data)
        with self.assertRaises(ValueError):
            # provide an invalid uncompressed size
            lz4.decompress_safe(compressed, int(input_data_size/2))

    def test_compress_bound(self):
        # The value of 17 could change with future versions of lz4.
        self.assertEqual(lz4.compress_bound(1), 17)

    def test_compress_bound_invalid(self):
        with self.assertRaises(ValueError):
            # 0x7E000000 is currently the max allowed by lz4
            lz4.compress_bound(0x7E000000+1)

    def test_create_and_free_stream(self):
        stream = lz4.create_stream(4*1024)
        self.assertNotEqual(stream, None)
        lz4.free_stream(stream)

    def test_create_and_free_decode_stream(self):
        stream = lz4.create_decode_stream(4*1024)
        self.assertNotEqual(stream, None)
        lz4.free_decode_stream(stream)

    def test_stream_compress(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        block_size = int((len(input_data)/2)+1)

        stream = lz4.create_stream(block_size)
        self.assertNotEqual(stream, None)

        compressed_data1 = lz4.compress_fast_continue(stream, input_data[:block_size])
        compressed_data2 = lz4.compress_fast_continue(stream, input_data[block_size:])

        lz4.free_stream(stream)

        stream = lz4.create_decode_stream(block_size)
        self.assertNotEqual(stream, None)

        decompressed_data1 = lz4.decompress_safe_continue(stream, compressed_data1)
        decompressed_data2 = lz4.decompress_safe_continue(stream, compressed_data2)

        lz4.free_decode_stream(stream)

        decompressed_data = decompressed_data1+decompressed_data2
        self.assertEqual(decompressed_data, input_data)


if __name__ == '__main__':
    unittest.main()

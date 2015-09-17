from lz4ex import lz4, lz4hc
import unittest


class TestLZ4(unittest.TestCase):
    def test_compress_default(self):
        input_data = "2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        input_data_size = len(input_data)
        compressed = lz4hc.compress_hc(input_data, lz4hc.COMPRESSIONLEVEL_MAX)
        decompressed = lz4.decompress_safe(compressed, input_data_size)
        self.assertEqual(input_data, decompressed)

    def test_create_and_free_stream(self):
        stream = lz4hc.create_hc_stream(4*1024, lz4hc.COMPRESSIONLEVEL_MAX)
        self.assertNotEqual(stream, None)
        lz4hc.free_hc_stream(stream)

    def test_stream_compress(self):
        input_data = "2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        block_size = (len(input_data)/2)+1

        stream = lz4hc.create_hc_stream(block_size, lz4hc.COMPRESSIONLEVEL_MAX)
        self.assertNotEqual(stream, None)

        compressed_data1 = lz4hc.compress_hc_continue(stream, input_data[:block_size])
        compressed_data2 = lz4hc.compress_hc_continue(stream, input_data[block_size:])

        lz4hc.free_hc_stream(stream)

        stream = lz4.create_decode_stream(block_size)
        self.assertNotEqual(stream, None)

        decompressed_data1 = lz4.decompress_safe_continue(stream, compressed_data1)
        decompressed_data2 = lz4.decompress_safe_continue(stream, compressed_data2)

        lz4.free_decode_stream(stream)

        decompressed_data = decompressed_data1+decompressed_data2
        self.assertEqual(decompressed_data, input_data)


if __name__ == '__main__':
    unittest.main()

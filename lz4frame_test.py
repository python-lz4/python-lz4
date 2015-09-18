from lz4ex import lz4frame
import unittest

class TestLZ4Frame(unittest.TestCase):
    def test_create_and_free_compression_context(self):
        context = lz4frame.create_compression_context()
        self.assertNotEqual(context, None)
        lz4frame.free_compression_context(context)

    def test_compress_frame(self):
        input_data = "2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        compressed = lz4frame.compress_frame(input_data)
        decompressed = lz4frame.decompress(compressed)
        self.assertEqual(input_data, decompressed)

    def test_compress_begin_update_end(self):
        context = lz4frame.create_compression_context()
        self.assertNotEqual(context, None)

        input_data = "2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        chunk_size = int((len(input_data)/2)+1)
        compressed = lz4frame.compress_begin(context)
        compressed += lz4frame.compress_update(context, input_data[:chunk_size])
        compressed += lz4frame.compress_update(context, input_data[chunk_size:])
        compressed += lz4frame.compress_end(context)
        lz4frame.free_compression_context(context)

        decompressed = lz4frame.decompress(compressed,1024)
        self.assertEqual(input_data, decompressed)


    def test_get_frame_info(self):
        input_data = "2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123"
        compressed = lz4frame.compress_frame(
            input_data,
            len(input_data),
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


if __name__ == '__main__':
    unittest.main()

import lz4.block
import sys


from multiprocessing.pool import ThreadPool
import unittest
import os

class TestLZ4Block(unittest.TestCase):

    def test_random(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.block.decompress(lz4.compress(DATA)))

    def test_random_hc1(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.block.decompress(
          lz4.compress(DATA, mode='high_compression')))

    def test_random_hc2(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.block.decompress(
          lz4.compress(DATA, mode='high_compression', compression=7)))

    def test_random_hc3(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.block.decompress(
          lz4.compress(DATA, mode='high_compression', compression=16)))

    def test_random_fast1(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.block.decompress(
          lz4.compress(DATA, mode='fast')))

    def test_random_fast2(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.block.decompress(
          lz4.compress(DATA, mode='fast', acceleration=5)))

    def test_random_fast3(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.block.decompress(
          lz4.compress(DATA, mode='fast', acceleration=9)))

    def test_random_no_store_size(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb

      self.assertEqual(
          DATA, lz4.block.decompress(
              lz4.compress(DATA, store_size=False),
              uncompressed_size=128 * 1024
          )
      )

    def test_random_hc_no_store_size(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb

      self.assertEqual(
          DATA, lz4.block.decompress(
              lz4.compress(DATA, mode='high_compression', compression=9, store_size=False),
              uncompressed_size=128 * 1024
          )
      )

    def test_random_fast_no_store_size(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb

      self.assertEqual(
          DATA, lz4.block.decompress(
              lz4.compress(DATA, mode='fast', acceleration=9, store_size=False),
              uncompressed_size=128 * 1024
          )
      )

    def test_threads(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtrip(x):
            return lz4.decompress(lz4.compress(x))

        pool = ThreadPool(8)
        out = pool.map(roundtrip, data)
        assert data == out
        pool.close()

    def test_threads_hc1(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtriphc(x):
            return lz4.decompress(lz4.compress(x, mode='high_compression'))

        pool = ThreadPool(8)
        out = pool.map(roundtriphc, data)
        assert data == out
        pool.close()

    def test_threads_hc2(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtriphc(x):
            return lz4.decompress(
                lz4.compress(x, mode='high_compression', compression=4))

        pool = ThreadPool(8)
        out = pool.map(roundtriphc, data)
        assert data == out
        pool.close()

    def test_threads_hc3(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtriphc(x):
            return lz4.decompress(
                lz4.compress(x, mode='high_compression', compression=9))

        pool = ThreadPool(8)
        out = pool.map(roundtriphc, data)
        assert data == out
        pool.close()

    def test_threads_fast1(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtripfast(x):
            return lz4.decompress(lz4.compress(x, mode='fast', acceleration=1))

        pool = ThreadPool(8)
        out = pool.map(roundtripfast, data)
        assert data == out
        pool.close()

    def test_threads_fast2(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtripfast(x):
            return lz4.decompress(lz4.compress(x, mode='fast', acceleration=4))

        pool = ThreadPool(8)
        out = pool.map(roundtripfast, data)
        assert data == out
        pool.close()

    def test_threads_fast3(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtripfast(x):
            return lz4.decompress(lz4.compress(x, mode='fast', acceleration=8))

        pool = ThreadPool(8)
        out = pool.map(roundtripfast, data)
        assert data == out
        pool.close()

    def test_threads_fast_no_store_size(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtripfast(x):
            return lz4.decompress(
                lz4.compress(x, mode='fast', acceleration=8, store_size=False),
                uncompressed_size=128 * 1024
            )

        pool = ThreadPool(8)
        out = pool.map(roundtripfast, data)
        assert data == out
        pool.close()

    def test_threads_hc_no_store_size(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtriphc(x):
            return lz4.decompress(
                lz4.compress(x, mode='high_compression', store_size=False, compression=4),
                uncompressed_size=128 * 1024
            )

        pool = ThreadPool(8)
        out = pool.map(roundtriphc, data)
        assert data == out
        pool.close()

    def test_block_format(self):
        data = lz4.compress(b'A' * 64)
        self.assertEqual(data[:4], b'\x40\0\0\0')
        self.assertEqual(lz4.decompress(data[4:], uncompressed_size=64), b'A' * 64)

class TestLZ4BlockModern(unittest.TestCase):
    def test_decompress_ui32_overflow(self):
        data = lz4.compress(b'A' * 64)
        with self.assertRaisesRegexp(OverflowError, r'^signed integer is greater than maximum$'):
            lz4.decompress(data[4:], uncompressed_size=((1<<32) + 64))

    def test_decompress_without_leak(self):
        # Verify that hand-crafted packet does not leak uninitialized(?) memory.
        data = lz4.compress(b'A' * 64)
        with self.assertRaisesRegexp(ValueError, r'^Decompressor writes 64 bytes, but 79 are claimed$'):
            lz4.decompress(b'\x4f' + data[1:])
        with self.assertRaisesRegexp(ValueError, r'^Decompressor writes 64 bytes, but 79 are claimed$'):
            lz4.decompress(data[4:], uncompressed_size=79)

    def test_decompress_with_trailer(self):
        data = b'A' * 64
        comp = lz4.compress(data)
        with self.assertRaisesRegexp(ValueError, r'^Corrupt input at byte'):
            self.assertEqual(data, lz4.block.decompress(comp + b'A'))
        with self.assertRaisesRegexp(ValueError, r'^Corrupt input at byte'):
            self.assertEqual(data, lz4.block.decompress(comp + comp))
        with self.assertRaisesRegexp(ValueError, r'^Corrupt input at byte'):
            self.assertEqual(data, lz4.block.decompress(comp + comp[4:]))

if sys.version_info < (2, 7):
    # Poor-man unittest.TestCase.skip for Python 2.6
    del TestLZ4BlockModern

if __name__ == '__main__':
    unittest.main()


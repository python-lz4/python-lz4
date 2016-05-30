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
          lz4.compress(DATA, mode='high_compression', acceleration=0,compression=7)))

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

if __name__ == '__main__':
    unittest.main()


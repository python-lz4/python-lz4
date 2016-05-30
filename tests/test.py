import lz4.block
import sys


from multiprocessing.pool import ThreadPool
import unittest
import os

class TestLZ4Block(unittest.TestCase):

    def test_random(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.block.decompress(lz4.compress(DATA)))
      self.assertEqual(DATA, lz4.block.decompress(lz4.compress(DATA, mode='high_compression')))
      self.assertEqual(DATA, lz4.block.decompress(lz4.compress(DATA, mode='fast')))
      self.assertEqual(DATA, lz4.block.decompress(lz4.compress(DATA, mode='fast', acceleration=5)))
      self.assertEqual(DATA, lz4.block.decompress(lz4.compress(DATA, mode='fast', acceleration=9)))

    def test_threads(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtrip(x):
            return lz4.decompress(lz4.compress(x))

        pool = ThreadPool(8)
        out = pool.map(roundtrip, data)
        assert data == out
        pool.close()


if __name__ == '__main__':
    unittest.main()


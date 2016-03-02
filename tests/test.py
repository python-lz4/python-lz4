import lz4
import sys


from multiprocessing.pool import ThreadPool
import unittest
import os

class TestLZ4(unittest.TestCase):

    def test_random(self):
      DATA = os.urandom(128 * 1024)  # Read 128kb
      self.assertEqual(DATA, lz4.loads(lz4.dumps(DATA)))

    def test_threads(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtrip(x):
            return lz4.loads(lz4.dumps(x))

        pool = ThreadPool(8)
        out = pool.map(roundtrip, data)
        assert data == out
        pool.close()


if __name__ == '__main__':
    unittest.main()


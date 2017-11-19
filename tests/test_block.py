import lz4.block
import sys
from multiprocessing.pool import ThreadPool
import unittest
import os
import pytest


@pytest.fixture(
    params=[
        (b''),
        (os.urandom(128 * 1024)),
    ]
)
def data(request):
    return request.param

@pytest.fixture(
    params=[
        (True),
        (False),
    ]
)
def store_size(request):
    return request.param

@pytest.fixture(
    params=[
        ('fast'),
        ('high_compression')
    ]
)
def mode(request):
    return request.param

@pytest.fixture(
    params=[
        (i) for i in range(17)
    ]
)
def compression(request):
    return request.param

@pytest.fixture(
    params=[
        (i) for i in range(10)
    ]
)
def acceleration(request):
    return request.param

@pytest.fixture(
    params=[
        (i) for i in range(17)
    ]
)
def compression(request):
    return request.param

# Test defaults
def test_default(data):
    c = lz4.block.compress(data)
    d = lz4.block.decompress(c)
    assert(d == data)

# With and without store_size
def test_store_size(data, store_size):
    c = lz4.block.compress(data, store_size=store_size)
    if store_size:
        d = lz4.block.decompress(c)
    else:
        d = lz4.block.decompress(c, uncompressed_size=len(data))
    assert(d == data)

# Specify mode only
def test_mode_defaults(data, mode, store_size):
    c = lz4.block.compress(data, mode=mode, store_size=store_size)
    if store_size:
        d = lz4.block.decompress(c)
    else:
        d = lz4.block.decompress(c, uncompressed_size=len(data))
    assert(d == data)

# Test high compression mode
def test_high_compression(data, compression, store_size):
    c = lz4.block.compress(data, mode='high_compression', store_size=store_size)
    if store_size:
        d = lz4.block.decompress(c)
    else:
        d = lz4.block.decompress(c, uncompressed_size=len(data))
    assert(d == data)

# Test fast compression mode
def test_fast(data, acceleration, store_size):
    c = lz4.block.compress(data, mode='fast', store_size=store_size)
    if store_size:
        d = lz4.block.decompress(c)
    else:
        d = lz4.block.decompress(c, uncompressed_size=len(data))
    assert(d == data)


class TestLZ4Block(unittest.TestCase):

    def test_threads(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtrip(x):
            return lz4.block.decompress(lz4.block.compress(x))

        pool = ThreadPool(8)
        out = pool.map(roundtrip, data)
        assert data == out
        pool.close()

    def test_threads_hc1(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtriphc(x):
            return lz4.block.decompress(lz4.block.compress(x, mode='high_compression'))

        pool = ThreadPool(8)
        out = pool.map(roundtriphc, data)
        assert data == out
        pool.close()

    def test_threads_hc2(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtriphc(x):
            return lz4.block.decompress(
                lz4.block.compress(x, mode='high_compression', compression=4))

        pool = ThreadPool(8)
        out = pool.map(roundtriphc, data)
        assert data == out
        pool.close()

    def test_threads_hc3(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtriphc(x):
            return lz4.block.decompress(
                lz4.block.compress(x, mode='high_compression', compression=9))

        pool = ThreadPool(8)
        out = pool.map(roundtriphc, data)
        assert data == out
        pool.close()

    def test_threads_fast1(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtripfast(x):
            return lz4.block.decompress(lz4.block.compress(x, mode='fast', acceleration=1))

        pool = ThreadPool(8)
        out = pool.map(roundtripfast, data)
        assert data == out
        pool.close()

    def test_threads_fast2(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtripfast(x):
            return lz4.block.decompress(lz4.block.compress(x, mode='fast', acceleration=4))

        pool = ThreadPool(8)
        out = pool.map(roundtripfast, data)
        assert data == out
        pool.close()

    def test_threads_fast3(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtripfast(x):
            return lz4.block.decompress(lz4.block.compress(x, mode='fast', acceleration=8))

        pool = ThreadPool(8)
        out = pool.map(roundtripfast, data)
        assert data == out
        pool.close()

    def test_threads_fast_no_store_size(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtripfast(x):
            return lz4.block.decompress(
                lz4.block.compress(x, mode='fast', acceleration=8, store_size=False),
                uncompressed_size=128 * 1024
            )

        pool = ThreadPool(8)
        out = pool.map(roundtripfast, data)
        assert data == out
        pool.close()

    def test_threads_hc_no_store_size(self):
        data = [os.urandom(128 * 1024) for i in range(100)]
        def roundtriphc(x):
            return lz4.block.decompress(
                lz4.block.compress(x, mode='high_compression', store_size=False, compression=4),
                uncompressed_size=128 * 1024
            )

        pool = ThreadPool(8)
        out = pool.map(roundtriphc, data)
        assert data == out
        pool.close()

    def test_block_format(self):
        data = lz4.block.compress(b'A' * 64)
        self.assertEqual(data[:4], b'\x40\0\0\0')
        self.assertEqual(lz4.block.decompress(data[4:], uncompressed_size=64), b'A' * 64)

class TestLZ4BlockModern(unittest.TestCase):
    def test_decompress_ui32_overflow(self):
        data = lz4.block.compress(b'A' * 64)
        with self.assertRaises(OverflowError):
            lz4.block.decompress(data[4:], uncompressed_size=((1<<32) + 64))

    def test_decompress_without_leak(self):
        # Verify that hand-crafted packet does not leak uninitialized(?) memory.
        data = lz4.block.compress(b'A' * 64)
        with self.assertRaisesRegexp(ValueError, r'^Decompressor wrote 64 bytes, but 79 bytes expected from header$'):
            lz4.block.decompress(b'\x4f' + data[1:])
        with self.assertRaisesRegexp(ValueError, r'^Decompressor wrote 64 bytes, but 79 bytes expected from header$'):
            lz4.block.decompress(data[4:], uncompressed_size=79)

    def test_decompress_truncated(self):
        input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123" * 24
        compressed = lz4.block.compress(input_data)
        for i in range(len(compressed)):
            with self.assertRaisesRegexp(ValueError, '^(Input source data size too small|Corrupt input at byte \d+|Decompressor wrote \d+ bytes, but \d+ bytes expected from header)'):
                lz4.block.decompress(compressed[:i])

    def test_decompress_with_trailer(self):
        data = b'A' * 64
        comp = lz4.block.compress(data)
        with self.assertRaisesRegexp(ValueError, r'^Corrupt input at byte'):
            self.assertEqual(data, lz4.block.decompress(comp + b'A'))
        with self.assertRaisesRegexp(ValueError, r'^Corrupt input at byte'):
            self.assertEqual(data, lz4.block.decompress(comp + comp))
        with self.assertRaisesRegexp(ValueError, r'^Corrupt input at byte'):
            self.assertEqual(data, lz4.block.decompress(comp + comp[4:]))

if sys.version_info < (2, 7):
    # Poor-man unittest.TestCase.skip for Python 2.6
    del TestLZ4BlockModern


class TestLZ4BlockBufferObjects(unittest.TestCase):

    def test_bytearray(self):
        DATA = os.urandom(128 * 1024)  # Read 128kb
        compressed = lz4.block.compress(DATA)
        self.assertEqual(lz4.block.compress(bytearray(DATA)), compressed)
        self.assertEqual(lz4.block.decompress(bytearray(compressed)), DATA)

    def test_return_bytearray(self):
        if sys.version_info < (3,):
            return  # skip
        DATA = os.urandom(128 * 1024)  # Read 128kb
        compressed = lz4.block.compress(DATA)
        b = lz4.block.compress(DATA, return_bytearray=True)
        self.assertEqual(type(b), bytearray)
        self.assertEqual(bytes(b), compressed)
        b = lz4.block.decompress(compressed, return_bytearray=True)
        self.assertEqual(type(b), bytearray)
        self.assertEqual(bytes(b), DATA)

    def test_memoryview(self):
        if sys.version_info < (2, 7):
            return  # skip
        DATA = os.urandom(128 * 1024)  # Read 128kb
        compressed = lz4.block.compress(DATA)
        self.assertEqual(lz4.block.compress(memoryview(DATA)), compressed)
        self.assertEqual(lz4.block.decompress(memoryview(compressed)), DATA)

    def test_unicode(self):
        if sys.version_info < (3,):
            return  # skip
        DATA = b'x'
        self.assertRaises(TypeError, lz4.block.compress, DATA.decode('latin1'))
        self.assertRaises(TypeError, lz4.block.decompress,
                          lz4.block.compress(DATA).decode('latin1'))


if __name__ == '__main__':
    unittest.main()

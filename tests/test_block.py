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
        (b'0' * 128 * 1024),
    ]
)
def data(request):
    return request.param

@pytest.fixture(
    params=[
        (
            {
                'store_size': True
            }
        ),
        (
            {
                'store_size': False
            }
        ),
    ]
)
def store_size(request):
    return request.param

@pytest.fixture(
    params=[
        ('fast', None)
    ] + [
        ('fast', {'acceleration': s}) for s in range(10)
    ] + [
        ('high_compression', None)
    ] + [
        ('high_compression', {'compression': s}) for s in range(17)
    ] + [
        (None, None)
    ]
)
def mode(request):
    return request.param

# Test single threaded usage with all valid variations of input
def test_1(data, mode, store_size):
    kwargs = {}

    if mode[0] != None:
        kwargs['mode'] = mode[0]
    if mode[1] != None:
        kwargs.update(mode[1])

    kwargs.update(store_size)

    c = lz4.block.compress(data, **kwargs)
    if store_size['store_size']:
        d = lz4.block.decompress(c)
    else:
        d = lz4.block.decompress(c, uncompressed_size=len(data))
    assert(d == data)

# Test multi threaded usage with all valid variations of input
def test_threads2(data, mode, store_size):
    kwargs = {}
    if mode[0] != None:
        kwargs['mode'] = mode[0]
    if mode[1] != None:
        kwargs.update(mode[1])

    kwargs.update(store_size)

    def roundtrip(x):
        c = lz4.block.compress(x, **kwargs)
        if store_size['store_size']:
            d = lz4.block.decompress(c)
        else:
            d = lz4.block.decompress(c, uncompressed_size=len(x))
        return d

    data_in = [data for i in range(32)]

    pool = ThreadPool(8)
    data_out = pool.map(roundtrip, data_in)
    pool.close()
    assert data_in == data_out




class TestLZ4Block(unittest.TestCase):

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

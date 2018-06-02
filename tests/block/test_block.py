import lz4.block
import sys
from multiprocessing.pool import ThreadPool
from functools import partial
import os
import pytest
if sys.version_info <= (3, 2):
    import struct


def get_stored_size(buff):
    if sys.version_info > (2, 7):
        if isinstance(buff, memoryview):
            b = buff.tobytes()
        else:
            b = bytes(buff)
    else:
        b = bytes(buff)

    if len(b) < 4:
        return None

    if sys.version_info > (3, 2):
        return int.from_bytes(b[:4], 'little')
    else:
        # This would not work on a memoryview object, hence buff.tobytes call
        # above
        return struct.unpack('<I', b[:4])[0]


def roundtrip(x, c_kwargs, d_kwargs, dictionary):
    if dictionary:
        if isinstance(dictionary, tuple):
            d = x[dictionary[0]:dictionary[1]]
        else:
            d = dictionary
        c_kwargs['dict'] = d
        d_kwargs['dict'] = d

    c = lz4.block.compress(x, **c_kwargs)

    if c_kwargs['store_size']:
        assert get_stored_size(c) == len(x)
    else:
        d_kwargs['uncompressed_size'] = len(x)

    return lz4.block.decompress(c, **d_kwargs)


def setup_kwargs(mode, store_size, c_return_bytearray=None, d_return_bytearray=None):
    c_kwargs = {}

    if mode[0] is not None:
        c_kwargs['mode'] = mode[0]
    if mode[1] is not None:
        c_kwargs.update(mode[1])

    c_kwargs.update(store_size)

    if(c_return_bytearray):
        c_kwargs.update(c_return_bytearray)

    d_kwargs = {}

    if(d_return_bytearray):
        d_kwargs.update(d_return_bytearray)

    return (c_kwargs, d_kwargs)


# Test single threaded usage with all valid variations of input
def test_1(data, mode, store_size, c_return_bytearray, d_return_bytearray, dictionary):
    (c_kwargs, d_kwargs) = setup_kwargs(mode, store_size, c_return_bytearray, d_return_bytearray)

    d = roundtrip(data, c_kwargs, d_kwargs, dictionary)

    assert d == data
    if d_return_bytearray['return_bytearray']:
        assert isinstance(d, bytearray)


# Test multi threaded usage with all valid variations of input
def test_threads2(data, mode, store_size, dictionary):
    (c_kwargs, d_kwargs) = setup_kwargs(mode, store_size)

    data_in = [data for i in range(32)]

    pool = ThreadPool(8)
    rt = partial(roundtrip, c_kwargs=c_kwargs, d_kwargs=d_kwargs, dictionary=dictionary)
    data_out = pool.map(rt, data_in)
    pool.close()
    assert data_in == data_out


def test_decompress_ui32_overflow():
    data = lz4.block.compress(b'A' * 64)
    with pytest.raises(OverflowError):
        lz4.block.decompress(data[4:], uncompressed_size=((1 << 32) + 64))


def test_decompress_without_leak():
    # Verify that hand-crafted packet does not leak uninitialized(?) memory.
    data = lz4.block.compress(b'A' * 64)
    message = r'^Decompressor wrote 64 bytes, but 79 bytes expected from header$'
    with pytest.raises(ValueError, match=message):
        lz4.block.decompress(b'\x4f' + data[1:])
    with pytest.raises(ValueError, match=message):
        lz4.block.decompress(data[4:], uncompressed_size=79)


def test_decompress_truncated():
    input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123" * 24
    compressed = lz4.block.compress(input_data)
    # for i in range(len(compressed)):
    #     try:
    #         lz4.block.decompress(compressed[:i])
    #     except:
    #         print(i, sys.exc_info()[0], sys.exc_info()[1])
    with pytest.raises(ValueError, match='Input source data size too small'):
        lz4.block.decompress(compressed[:0])
    for n in [0, 1]:
        with pytest.raises(ValueError, match='Input source data size too small'):
            lz4.block.decompress(compressed[:n])
    for n in [24, 25, -2, 27, 67, 85]:
        with pytest.raises(ValueError, match=r'Corrupt input at byte \d+|Decompressor wrote \d+ bytes, but \d+ bytes expected from header'):
            lz4.block.decompress(compressed[:n])


def test_decompress_with_trailer():
    data = b'A' * 64
    comp = lz4.block.compress(data)
    message=r'^Corrupt input at byte'
    with pytest.raises(ValueError, match=message):
        lz4.block.decompress(comp + b'A')
    with pytest.raises(ValueError, match=message):
        lz4.block.decompress(comp + comp)
    with pytest.raises(ValueError, match=message):
        lz4.block.decompress(comp + comp[4:])


def test_unicode():
    if sys.version_info < (3,):
        return  # skip
    DATA = b'x'
    with pytest.raises(TypeError):
        lz4.block.compress (DATA.decode('latin1'))
        lz4.block.decompress(lz4.block.compress(DATA).decode('latin1'))

# These next two are probably redundant given test_1 above but we'll keep them
# for now
def test_return_bytearray():
    if sys.version_info < (3,):
        return  # skip
    data = os.urandom(128 * 1024)  # Read 128kb
    compressed = lz4.block.compress(data)
    b = lz4.block.compress(data, return_bytearray=True)
    assert isinstance(b, bytearray)
    assert bytes(b) == compressed
    b = lz4.block.decompress(compressed, return_bytearray=True)
    assert isinstance(b, bytearray)
    assert bytes(b) == data

def test_memoryview():
    if sys.version_info < (2, 7):
        return  # skip
    data = os.urandom(128 * 1024)  # Read 128kb
    compressed = lz4.block.compress(data)
    assert lz4.block.compress(memoryview(data)) == compressed
    assert lz4.block.decompress(memoryview(compressed)) == data

def test_with_dict_none():
    input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123" * 24
    for mode in ['default', 'high_compression']:
        assert lz4.block.decompress(lz4.block.compress(input_data, mode=mode, dict=None)) == input_data
        assert lz4.block.decompress(lz4.block.compress(input_data, mode=mode), dict=None) == input_data
        assert lz4.block.decompress(lz4.block.compress(input_data, mode=mode, dict=b'')) == input_data
        assert lz4.block.decompress(lz4.block.compress(input_data, mode=mode), dict=b'') == input_data
        assert lz4.block.decompress(lz4.block.compress(input_data, mode=mode, dict='')) == input_data
        assert lz4.block.decompress(lz4.block.compress(input_data, mode=mode), dict='') == input_data

def test_with_dict():
    input_data = b"2099023098234882923049823094823094898239230982349081231290381209380981203981209381238901283098908123109238098123" * 24
    dict1 = input_data[10:30]
    dict2 = input_data[20:40]
    for mode in ['default', 'high_compression']:
        compressed = lz4.block.compress(input_data, mode=mode, dict=dict1)
        with pytest.raises(ValueError, match=r'Corrupt input at byte \d+'):
            lz4.block.decompress(compressed)
        with pytest.raises(ValueError, match=r'Corrupt input at byte \d+'):
            lz4.block.decompress(compressed, dict=dict1[:2])
        assert lz4.block.decompress(compressed, dict=dict2) != input_data
        assert lz4.block.decompress(compressed, dict=dict1) == input_data
    assert lz4.block.decompress(lz4.block.compress(input_data), dict=dict1) == input_data

def test_known_decompress():
    assert(lz4.block.decompress(
        b'\x00\x00\x00\x00\x00') ==
        b'')
    assert(lz4.block.decompress(
        b'\x01\x00\x00\x00\x10 ') ==
        b' ')
    assert(lz4.block.decompress(
        b'h\x00\x00\x00\xff\x0bLorem ipsum dolor sit amet\x1a\x006P amet') ==
        b'Lorem ipsum dolor sit amet' * 4)
    assert(lz4.block.decompress(
        b'\xb0\xb3\x00\x00\xff\x1fExcepteur sint occaecat cupidatat non proident.\x00' + (b'\xff' * 180) + b'\x1ePident') ==
        b'Excepteur sint occaecat cupidatat non proident' * 1000)


# @pytest.mark.skipif(sys.maxsize < 0xffffffff,
#                     reason='Py_ssize_t too small for this test')
# def test_huge():
#     try:
#         huge = b'\0' * 0x100000000  # warning: this allocates 4GB of memory!
#     except MemoryError:
#         pytest.skip('Insufficient system memory for this test')

#     with pytest.raises(
#             OverflowError, match='Input too large for LZ4 API'
#     ):
#         lz4.block.compress(huge)
#     with pytest.raises(
#             OverflowError, match='Dictionary too large for LZ4 API'
#     ):
#         lz4.block.compress(b'', dict=huge)
#     with pytest.raises(
#             OverflowError, match='Input too large for LZ4 API'
#     ):
#         lz4.block.decompress(huge)
#     with pytest.raises(
#             OverflowError, match='Dictionary too large for LZ4 API'
#     ):
#             lz4.block.decompress(b'', dict=huge)

import os
from concurrent.futures import ThreadPoolExecutor

import pytest

import lz4.block
import lz4


pytestmark = pytest.mark.skipif(
    lz4.library_version_number() < 10902,
    reason='compress_dest_size requires LZ4 1.9.2 or newer',
)


@pytest.mark.parametrize('factory', [bytes, bytearray, memoryview])
@pytest.mark.parametrize('target_size', [1, 2, 16, 64, 4096])
@pytest.mark.parametrize('data', [b'', b'abcd' * 256, bytes(range(256)) * 4])
def test_roundtrip(factory, target_size, data):
    source = factory(data)
    compressed, consumed = lz4.block.compress_dest_size(source, target_size)
    assert isinstance(compressed, bytes)
    assert 0 <= consumed <= len(data)
    assert len(compressed) <= target_size
    assert lz4.block.decompress(compressed, uncompressed_size=consumed) == data[:consumed]
    if target_size == 4096:
        assert consumed == len(data)
    if isinstance(source, bytearray):
        source.extend(b'x')  # No exported buffer may survive the call.


def test_compress_all_input_in_bounded_blocks():
    source = os.urandom(16384)
    offset = 0
    blocks = []
    while offset < len(source):
        block, consumed = lz4.block.compress_dest_size(
            memoryview(source)[offset:], target_size=128, return_bytearray=True,
        )
        assert isinstance(block, bytearray)
        assert 0 < consumed <= len(source) - offset
        assert len(block) <= 128
        blocks.append(lz4.block.decompress(block, uncompressed_size=consumed))
        offset += consumed
    assert b''.join(blocks) == source


@pytest.mark.parametrize('size,error', [
    (0, ValueError), (-1, ValueError), (2 ** 31, OverflowError),
    ('64', TypeError), (1.5, TypeError),
])
def test_invalid_target_releases_source(size, error):
    source = bytearray(b'data')
    with pytest.raises(error):
        lz4.block.compress_dest_size(source, size)
    source.extend(b'x')


def test_rejects_noncontiguous_input():
    with pytest.raises(BufferError):
        lz4.block.compress_dest_size(memoryview(b'abcdef')[::2], 64)


def test_parallel_calls():
    def roundtrip(data):
        block, consumed = lz4.block.compress_dest_size(data, 64)
        return lz4.block.decompress(block, uncompressed_size=consumed) == data[:consumed]
    with ThreadPoolExecutor(max_workers=4) as executor:
        assert all(executor.map(roundtrip, [os.urandom(1024) for _ in range(32)]))


def test_input_above_lz4_limit_releases_buffer():
    import mmap
    # Anonymous mapping reserves address space without materializing a 2 GB bytes object.
    with mmap.mmap(-1, 0x7E000001) as source:
        with pytest.raises(OverflowError, match='Input too large'):
            lz4.block.compress_dest_size(source, 64)


def test_large_target_does_not_require_large_allocation():
    block, consumed = lz4.block.compress_dest_size(b'data', 2 ** 31 - 1)
    assert consumed == 4
    assert lz4.block.decompress(block, uncompressed_size=consumed) == b'data'

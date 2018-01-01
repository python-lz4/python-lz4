from . helpers import (
    roundtrip_LZ4FrameCompressor,
    roundtrip_LZ4FrameCompressor_LZ4FrameDecompressor,
    decompress_truncated,
)
import os
import pytest

test_data=[
    b'',
    (128 * (32 * os.urandom(32))),
    (256 * (32 * os.urandom(32))),
    (512 * (32 * os.urandom(32))),
    (1024 * (32 * os.urandom(32))),
]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param

@pytest.fixture(
    params=[
        (True),
        (False)
    ]
)
def reset(request):
    return request.param

@pytest.fixture(
    params=[
        (1),
        (8)
    ]
)
def chunks(request):
    return request.param


def test_roundtrip_LZ4FrameCompressor(data, chunks, block_size, reset, block_checksum, content_checksum):
    roundtrip_LZ4FrameCompressor(
        data,
        chunks=chunks,
        block_size=block_size,
        reset=reset,
        block_checksum=block_checksum,
        content_checksum=content_checksum,
    )

def test_roundtrip_LZ4FrameCompressor_LZ4FrameDecompressor(
        data, chunks, block_size, reset, block_checksum, content_checksum):
    roundtrip_LZ4FrameCompressor_LZ4FrameDecompressor(
        data,
        chunks=chunks,
        block_size=block_size,
        reset=reset,
        block_checksum=block_checksum,
        content_checksum=content_checksum,
    )

def test_decompress_truncated(data):
    decompress_truncated(data)


import struct
import lz4.frame as lz4frame


def test_content_checksum_failure(data):
    compressed = lz4frame.compress(data, content_checksum=True)
    message = r'^LZ4F_decompress failed with code: ERROR_contentChecksum_invalid$'
    with pytest.raises(RuntimeError, message=message):
        last = struct.unpack('B', compressed[-1:])[0]
        lz4frame.decompress(compressed[:-1] + struct.pack('B', last ^ 0x42))

def test_block_checksum_failure(data):
    compressed = lz4frame.compress(
        data,
        content_checksum=True,
        block_checksum=True,
    )
    message = r'^LZ4F_decompress failed with code: ERROR_blockChecksum_invalid$'
    if len(compressed) > 64:
        with pytest.raises(RuntimeError, message=message):
            lz4frame.decompress(
                compressed[0:31] +
                struct.pack('B', compressed[32] ^ 0x42) +
                compressed[33:]
            )

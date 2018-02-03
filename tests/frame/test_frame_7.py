import lz4.frame as lz4frame
import pytest
import os

test_data=[
    (os.urandom(32) * 256),
]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param

def test_roundtrip_multiframe_1(data):
    nframes = 4

    compressed = b''
    for _ in range(nframes):
        compressed += lz4frame.compress(data)

    decompressed = b''
    for _ in range(nframes):
        decompressed += lz4frame.decompress(compressed)

    assert len(decompressed) == nframes * len(data)
    assert data * nframes == decompressed

def test_roundtrip_multiframe_2(data):
    nframes = 4

    compressed = b''
    ctx = lz4frame.create_compression_context()
    for _ in range(nframes):
        compressed += lz4frame.compress_begin(ctx)
        compressed += lz4frame.compress_chunk(ctx, data)
        compressed += lz4frame.compress_flush(ctx)

    decompressed = b''
    for _ in range(nframes):
        decompressed += lz4frame.decompress(compressed)

    assert len(decompressed) == nframes * len(data)
    assert data * nframes == decompressed

def test_roundtrip_multiframe_3(data):
    nframes = 4

    compressed = b''
    ctx = lz4frame.create_compression_context()
    for _ in range(nframes):
        compressed += lz4frame.compress_begin(ctx)
        compressed += lz4frame.compress_chunk(ctx, data)
        compressed += lz4frame.compress_flush(ctx)

    decompressed = b''
    ctx = lz4frame.create_decompression_context()
    for _ in range(nframes):
        d, bytes_read, eof = lz4frame.decompress_chunk(ctx, compressed)
        decompressed += d
        assert eof == True
        assert bytes_read == len(compressed) // nframes

    assert len(decompressed) == nframes * len(data)
    assert data * nframes == decompressed

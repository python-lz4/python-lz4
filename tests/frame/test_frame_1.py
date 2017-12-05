import lz4.frame as lz4frame
import pytest

def test_create_compression_context():
    context = lz4frame.create_compression_context()
    assert context != None

def test_create_decompression_context():
    context = lz4frame.create_decompression_context()
    assert context != None

# TODO add source_size fixture
def test_roundtrip(data, block_size, block_mode,
                   content_checksum, frame_type,
                   compression_level, auto_flush):
    c_context = lz4frame.create_compression_context()
    compressed = lz4frame.compress_begin(
        c_context,
        source_size=len(data),
        compression_level=compression_level,
        block_size=block_size,
        content_checksum=content_checksum,
        frame_type=frame_type,
        auto_flush=auto_flush
    )
    compressed += lz4frame.compress_update(
        c_context,
        data)
    compressed += lz4frame.compress_end(c_context)
    d_context = lz4frame.create_decompression_context()
    decompressed, bytes_read = lz4frame.decompress(d_context, compressed)
    assert bytes_read == len(compressed)
    assert decompressed == data

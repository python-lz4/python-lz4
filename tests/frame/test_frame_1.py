import lz4.frame as lz4frame
import pytest

def test_create_compression_context():
    context = lz4frame.create_compression_context()
    assert context != None

def test_create_decompression_context():
    context = lz4frame.create_decompression_context()
    assert context != None

def test_roundtrip_1(data, block_size, block_mode,
                     content_checksum, frame_type,
                     compression_level, store_size):
    compressed = lz4frame.compress(
        data,
        store_size=store_size,
        compression_level=compression_level,
        block_size=block_size,
        block_mode=block_mode,
        content_checksum=content_checksum,
        frame_type=frame_type,
    )
    decompressed, bytes_read = lz4frame.decompress(compressed)
    assert bytes_read == len(compressed)
    assert decompressed == data

# TODO add source_size fixture
def test_roundtrip_2(data, block_size, block_mode,
                     content_checksum, frame_type,
                     compression_level, auto_flush):
    c_context = lz4frame.create_compression_context()
    compressed = lz4frame.compress_begin(
        c_context,
        source_size=len(data),
        compression_level=compression_level,
        block_size=block_size,
        block_mode=block_mode,
        content_checksum=content_checksum,
        frame_type=frame_type,
        auto_flush=auto_flush
    )
    compressed += lz4frame.compress_chunk(
        c_context,
        data)
    compressed += lz4frame.compress_end(c_context)
    decompressed, bytes_read = lz4frame.decompress(compressed)
    assert bytes_read == len(compressed)
    assert decompressed == data

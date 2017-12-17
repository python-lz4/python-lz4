import lz4.frame as lz4frame
import pytest

def test_create_compression_context():
    context = lz4frame.create_compression_context()
    assert context != None

def test_create_decompression_context():
    context = lz4frame.create_decompression_context()
    assert context != None

def test_roundtrip_1(data, block_size, block_mode,
                     content_checksum, compression_level,
                     store_size):
    compressed = lz4frame.compress(
        data,
        store_size=store_size,
        compression_level=compression_level,
        block_size=block_size,
        block_mode=block_mode,
        content_checksum=content_checksum,
    )

    frame_info = lz4frame.get_frame_info(compressed)

    assert frame_info["content_checksum"] == content_checksum

    assert frame_info["skippable"] == False

    if store_size is True:
        assert frame_info["content_size"] == len(data)
    else:
        assert frame_info["content_size"] == 0

    if len(data) > frame_info['block_size'] and block_mode == lz4frame.BLOCKMODE_LINKED:
        if block_mode ==lz4frame.BLOCKMODE_LINKED:
            assert frame_info["blocks_linked"] == True
        else:
            assert frame_info["blocks_linked"] == False
        if block_size == lz4frame.BLOCKSIZE_DEFAULT:
            assert frame_info["block_size_id"] == lz4frame.BLOCKSIZE_MAX64KB
        else:
            assert frame_info["block_size_id"] == block_size
    else:
        # If there's only a single block in the frame, then LZ4 lib will set
        # the block mode to be independent
        assert frame_info["blocks_linked"] == False

    decompressed, bytes_read = lz4frame.decompress(compressed)
    assert bytes_read == len(compressed)
    assert decompressed == data

def test_roundtrip_2(data, block_size, block_mode,
                     content_checksum, compression_level,
                     auto_flush, store_size):
    c_context = lz4frame.create_compression_context()

    kwargs = {}
    kwargs['compression_level'] = compression_level
    kwargs['block_size'] = block_size
    kwargs['block_mode'] = block_mode
    kwargs['content_checksum'] = content_checksum
    kwargs['auto_flush'] = auto_flush
    if store_size is True:
        kwargs['source_size'] = len(data)

    compressed = lz4frame.compress_begin(
        c_context,
        **kwargs
    )
    compressed += lz4frame.compress_chunk(
        c_context,
        data)
    compressed += lz4frame.compress_end(c_context)
    decompressed, bytes_read = lz4frame.decompress(compressed)
    assert bytes_read == len(compressed)
    assert decompressed == data

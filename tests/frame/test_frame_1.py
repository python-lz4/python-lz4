import lz4.frame as lz4frame

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

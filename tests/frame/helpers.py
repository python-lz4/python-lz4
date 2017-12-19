import lz4.frame as lz4frame
import math

def roundtrip_1(data, block_size, block_linked,
                content_checksum, compression_level,
                store_size):
    compressed = lz4frame.compress(
        data,
        store_size=store_size,
        compression_level=compression_level,
        block_size=block_size,
        block_linked=block_linked,
        content_checksum=content_checksum,
    )
    decompressed, bytes_read = lz4frame.decompress(compressed)
    assert bytes_read == len(compressed)
    assert decompressed == data

def roundtrip_2(data, block_size, block_linked,
                content_checksum, compression_level,
                auto_flush, store_size):
    c_context = lz4frame.create_compression_context()

    kwargs = {}
    kwargs['compression_level'] = compression_level
    kwargs['block_size'] = block_size
    kwargs['block_linked'] = block_linked
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
        data
    )
    compressed += lz4frame.compress_end(c_context)
    decompressed, bytes_read = lz4frame.decompress(compressed)
    assert bytes_read == len(compressed)
    assert decompressed == data


def get_chunked(data, nchunks):
    size = len(data)
    stride = int(math.ceil(float(size)/nchunks)) # no // on py 2.6
    start = 0
    end = start + stride
    while end < size:
        yield data[start:end]
        start += stride
        end += stride
    yield data[start:]

def roundtrip_chunked(data, block_size, block_linked,
                      content_checksum, compression_level,
                      auto_flush, store_size):
    data, c_chunks, d_chunks = data

    c_context = lz4frame.create_compression_context()

    kwargs = {}
    kwargs['compression_level'] = compression_level
    kwargs['block_size'] = block_size
    kwargs['block_linked'] = block_linked
    kwargs['content_checksum'] = content_checksum
    kwargs['auto_flush'] = auto_flush
    if store_size is True:
        kwargs['source_size'] = len(data)

    compressed = lz4frame.compress_begin(
        c_context,
        **kwargs
    )
    data_in = get_chunked(data, c_chunks)
    try:
        while True:
            compressed += lz4frame.compress_chunk(
                c_context,
                next(data_in)
            )
    except StopIteration:
        pass
    finally:
        del data_in

    compressed += lz4frame.compress_end(c_context)

    d_context = lz4frame.create_decompression_context()
    compressed_in = get_chunked(compressed, d_chunks)
    decompressed = b''
    bytes_read = 0
    try:
        while True:
            d, b = lz4frame.decompress_chunk(d_context, next(compressed_in))
            decompressed += d
            bytes_read += b
    except StopIteration:
        pass
    finally:
        del compressed_in

    #assert bytes_read == len(compressed)
    assert decompressed == data


def get_frame_info_1(data,
                     block_size=lz4frame.BLOCKSIZE_DEFAULT,
                     block_linked=True,
                     content_checksum=False,
                     compression_level=5,
                     store_size=True):
    compressed = lz4frame.compress(
        data,
        store_size=store_size,
        compression_level=compression_level,
        block_size=block_size,
        block_linked=block_linked,
        content_checksum=content_checksum,
    )

    frame_info = lz4frame.get_frame_info(compressed)

    assert frame_info["content_checksum"] == content_checksum

    assert frame_info["skippable"] == False

    if store_size is True:
        assert frame_info["content_size"] == len(data)
    else:
        assert frame_info["content_size"] == 0

    if len(data) > frame_info['block_size']:
        assert frame_info["block_linked"] == block_linked

        if block_size == lz4frame.BLOCKSIZE_DEFAULT:
            assert frame_info["block_size_id"] == lz4frame.BLOCKSIZE_MAX64KB
        else:
            assert frame_info["block_size_id"] == block_size
    else:
        # If there's only a single block in the frame, then LZ4 lib will set
        # the block mode to be independent
        assert frame_info["block_linked"] == False

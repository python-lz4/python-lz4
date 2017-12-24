import lz4.frame as lz4frame
import math


def get_frame_info_check(compressed_data,
                         source_size,
                         store_size,
                         block_size,
                         block_linked,
                         content_checksum,
                         block_checksum):

    frame_info = lz4frame.get_frame_info(compressed_data)

    assert frame_info["content_checksum"] == content_checksum
    assert frame_info["block_checksum"] == block_checksum

    assert frame_info["skippable"] == False

    if store_size is True:
        assert frame_info["content_size"] == source_size
    else:
        assert frame_info["content_size"] == 0

    if source_size > frame_info['block_size']:
        # More than a single block
        assert frame_info["block_linked"] == block_linked

        if block_size == lz4frame.BLOCKSIZE_DEFAULT:
            assert frame_info["block_size_id"] == lz4frame.BLOCKSIZE_MAX64KB
        else:
            assert frame_info["block_size_id"] == block_size


def roundtrip_1(data,
                block_size=lz4frame.BLOCKSIZE_DEFAULT,
                block_linked=True,
                content_checksum=False,
                block_checksum=False,
                compression_level=5,
                store_size=True):

    compressed = lz4frame.compress(
        data,
        store_size=store_size,
        compression_level=compression_level,
        block_size=block_size,
        block_linked=block_linked,
        content_checksum=content_checksum,
        block_checksum=block_checksum,
    )
    get_frame_info_check(
        compressed,
        len(data),
        store_size,
        block_size,
        block_linked,
        content_checksum,
        block_checksum,
    )
    decompressed, bytes_read = lz4frame.decompress(compressed, return_bytes_read=True)
    assert bytes_read == len(compressed)
    assert decompressed == data

def roundtrip_2(data,
                block_size=lz4frame.BLOCKSIZE_DEFAULT,
                block_linked=True,
                content_checksum=False,
                block_checksum=False,
                compression_level=5,
                auto_flush=False,
                store_size=True):

    c_context = lz4frame.create_compression_context()

    kwargs = {}
    kwargs['compression_level'] = compression_level
    kwargs['block_size'] = block_size
    kwargs['block_linked'] = block_linked
    kwargs['content_checksum'] = content_checksum
    kwargs['block_checksum'] = block_checksum
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
    compressed += lz4frame.compress_flush(c_context)
    get_frame_info_check(
        compressed,
        len(data),
        store_size,
        block_size,
        block_linked,
        content_checksum,
        block_checksum,
    )
    decompressed, bytes_read = lz4frame.decompress(compressed, return_bytes_read=True)
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


def roundtrip_chunked(data,
                      block_size=lz4frame.BLOCKSIZE_DEFAULT,
                      block_linked=True,
                      content_checksum=False,
                      block_checksum=False,
                      compression_level=5,
                      auto_flush=False,
                      store_size=True):

    data, c_chunks, d_chunks = data

    c_context = lz4frame.create_compression_context()

    kwargs = {}
    kwargs['compression_level'] = compression_level
    kwargs['block_size'] = block_size
    kwargs['block_linked'] = block_linked
    kwargs['content_checksum'] = content_checksum
    kwargs['block_checksum'] = block_checksum
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

    compressed += lz4frame.compress_flush(c_context)

    get_frame_info_check(
        compressed,
        len(data),
        store_size,
        block_size,
        block_linked,
        content_checksum,
        block_checksum,
    )

    d_context = lz4frame.create_decompression_context()
    compressed_in = get_chunked(compressed, d_chunks)
    decompressed = b''
    bytes_read = 0
    try:
        while True:
            d, b = lz4frame.decompress_chunk(
                d_context,
                next(compressed_in),
                return_bytes_read=True
            )
            decompressed += d
            bytes_read += b
    except StopIteration:
        pass
    finally:
        del compressed_in

    assert bytes_read == len(compressed)
    assert decompressed == data


def roundtrip_LZ4FrameCompressor(
        data,
        chunks=1,
        block_size=lz4frame.BLOCKSIZE_DEFAULT,
        block_linked=True,
        content_checksum=False,
        block_checksum=False,
        compression_level=5,
        auto_flush=False,
        store_size=True,
        reset=False):

    with lz4frame.LZ4FrameCompressor(
            block_size=block_size,
            block_linked=block_linked,
            content_checksum=content_checksum,
            block_checksum=block_checksum,
            compression_level=compression_level,
            auto_flush=auto_flush,
    ) as compressor:
        def do_compress():
            if store_size is True:
                compressed = compressor.compress_begin(source_size=len(data))
            else:
                compressed = compressor.compress_begin()

            for chunk in get_chunked(data, chunks):
                compressed += compressor.compress(chunk)

            compressed += compressor.finalize()
            return compressed

        compressed = do_compress()

        if reset is True:
            compressor.reset()
            compressed = do_compress()

    get_frame_info_check(
        compressed,
        len(data),
        store_size,
        block_size,
        block_linked,
        content_checksum,
        block_checksum,
    )

    decompressed, bytes_read = lz4frame.decompress(compressed, return_bytes_read=True)
    assert data == decompressed
    assert bytes_read == len(compressed)

def roundtrip_LZ4FrameCompressor_LZ4FrameDecompressor(
        data,
        chunks=1,
        block_size=lz4frame.BLOCKSIZE_DEFAULT,
        block_linked=True,
        content_checksum=False,
        block_checksum=False,
        compression_level=5,
        auto_flush=False,
        store_size=True,
        reset=False):

    with lz4frame.LZ4FrameCompressor(
            block_size=block_size,
            block_linked=block_linked,
            content_checksum=content_checksum,
            block_checksum=block_checksum,
            compression_level=compression_level,
            auto_flush=auto_flush,
    ) as compressor:
        def do_compress():
            if store_size is True:
                compressed = compressor.compress_begin(source_size=len(data))
            else:
                compressed = compressor.compress_begin()

            for chunk in get_chunked(data, chunks):
                compressed += compressor.compress(chunk)

            compressed += compressor.finalize()
            return compressed

        compressed = do_compress()

        if reset is True:
            compressor.reset()
            compressed = do_compress()

    get_frame_info_check(
        compressed,
        len(data),
        store_size,
        block_size,
        block_linked,
        content_checksum,
        block_checksum,
    )

    with lz4frame.LZ4FrameDecompressor(return_bytes_read=True) as decompressor:
        decompressed = b''
        bytes_read = 0
        for chunk in get_chunked(compressed, chunks):
            d, b = decompressor.decompress(chunk, full_frame=False)
            decompressed += d
            bytes_read += b

    assert data == decompressed
    assert bytes_read == len(compressed)

import lz4.frame as lz4frame

def test_create_compression_context():
    context = lz4frame.create_compression_context()
    assert context != None

def test_create_decompression_context():
    context = lz4frame.create_decompression_context()
    assert context != None

def test_compress_return_type_1():
    r = lz4frame.compress(b'', return_bytearray=False)
    assert isinstance(r, bytes)

def test_compress_return_type_2():
    r = lz4frame.compress(b'', return_bytearray=True)
    assert isinstance(r, bytearray)

def test_decompress_return_type_1():
    c = lz4frame.compress(b'', return_bytearray=False)
    r = lz4frame.decompress(
        c,
        return_bytearray=False,
        return_bytes_read=False
    )
    assert isinstance(r, bytes)

def test_decompress_return_type_2():
    c = lz4frame.compress(b'', return_bytearray=False)
    r = lz4frame.decompress(
        c,
        return_bytearray=True,
        return_bytes_read=False
    )
    assert isinstance(r, bytearray)

def test_decompress_return_type_3():
    c = lz4frame.compress(b'', return_bytearray=False)
    r = lz4frame.decompress(
        c,
        return_bytearray=False,
        return_bytes_read=True
    )
    assert isinstance(r, tuple)
    assert isinstance(r[0], bytes)
    assert isinstance(r[1], int)

def test_decompress_return_type_4():
    c = lz4frame.compress(b'', return_bytearray=False)
    r = lz4frame.decompress(
        c,
        return_bytearray=True,
        return_bytes_read=True
    )
    assert isinstance(r, tuple)
    assert isinstance(r[0], bytearray)
    assert isinstance(r[1], int)

def test_decompress_chunk_return_type_1():
    c = lz4frame.compress(b'', return_bytearray=False)
    d = lz4frame.create_decompression_context()
    r = lz4frame.decompress_chunk(
        d,
        c,
        return_bytearray=False,
        return_bytes_read=False
    )
    assert isinstance(r, bytes)

def test_decompress_chunk_return_type_2():
    c = lz4frame.compress(b'', return_bytearray=False)
    d = lz4frame.create_decompression_context()
    r = lz4frame.decompress_chunk(
        d,
        c,
        return_bytearray=True,
        return_bytes_read=False
    )
    assert isinstance(r, bytearray)

def test_decompress_chunk_return_type_3():
    c = lz4frame.compress(b'', return_bytearray=False)
    d = lz4frame.create_decompression_context()
    r = lz4frame.decompress_chunk(
        d,
        c,
        return_bytearray=False,
        return_bytes_read=True
    )
    assert isinstance(r, tuple)
    assert isinstance(r[0], bytes)
    assert isinstance(r[1], int)

def test_decompress_chunk_return_type_4():
    c = lz4frame.compress(b'', return_bytearray=False)
    d = lz4frame.create_decompression_context()
    r = lz4frame.decompress_chunk(
        d,
        c,
        return_bytearray=True,
        return_bytes_read=True
    )
    assert isinstance(r, tuple)
    assert isinstance(r[0], bytearray)
    assert isinstance(r[1], int)

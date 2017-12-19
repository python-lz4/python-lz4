import lz4.frame as lz4frame

def test_create_compression_context():
    context = lz4frame.create_compression_context()
    assert context != None

def test_create_decompression_context():
    context = lz4frame.create_decompression_context()
    assert context != None

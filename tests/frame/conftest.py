import pytest
import os
import sys
import lz4.frame as lz4frame


test_data=[
    (b''),
    (os.urandom(8 * 1024)),
    (b'0' * 8 * 1024),
    # (bytearray(b'')),
    # (bytearray(os.urandom(8 * 1024))),
]
# if sys.version_info > (2, 7):
#     test_data += [
#         (memoryview(b'')),
#         (memoryview(os.urandom(8 * 1024)))
#     ]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param


test_data_chunked=[
    (b'', 1, 1),
    (os.urandom(8 * 1024), 8, 1),
    (os.urandom(8 * 1024), 1, 8),
    (b'0' * 8 * 1024, 8, 1),
    (b'0' * 8 * 1024, 8, 1),
    # (bytearray(b'')),
    # (bytearray(os.urandom(8 * 1024))),
]
# if sys.version_info > (2, 7):
#     test_data += [
#         (memoryview(b'')),
#         (memoryview(os.urandom(8 * 1024)))
#     ]

@pytest.fixture(
    params=test_data_chunked,
    ids=[
        'data' + str(i) for i in range(len(test_data_chunked))
    ]
)
def data_chunked(request):
    return request.param


@pytest.fixture(
    params=[
        (lz4frame.BLOCKSIZE_DEFAULT),
        (lz4frame.BLOCKSIZE_MAX64KB),
        (lz4frame.BLOCKSIZE_MAX256KB),
        (lz4frame.BLOCKSIZE_MAX1MB),
        (lz4frame.BLOCKSIZE_MAX4MB),
    ]
)
def block_size(request):
    return request.param

@pytest.fixture(
    params=[
        (lz4frame.BLOCKMODE_LINKED),
        (lz4frame.BLOCKMODE_INDEPENDENT),
    ]
)
def block_mode(request):
    return request.param

@pytest.fixture(
    params=[
        (lz4frame.CONTENTCHECKSUM_DISABLED),
        (lz4frame.CONTENTCHECKSUM_ENABLED),
    ]
)
def content_checksum(request):
    return request.param

@pytest.fixture(
    params=[
        (lz4frame.FRAMETYPE_FRAME),
        (lz4frame.FRAMETYPE_SKIPPABLEFRAME),
    ]
)
def frame_type(request):
    return request.param

compression_levels = list(range(-5, 13)) + [
        lz4frame.COMPRESSIONLEVEL_MIN,
        lz4frame.COMPRESSIONLEVEL_MINHC,
        lz4frame.COMPRESSIONLEVEL_MAX,
    ]
compression_levels = [
#    (i) for i in compression_levels
    (6)
]
@pytest.fixture(
    params=compression_levels
)
def compression_level(request):
    return request.param

@pytest.fixture(
    params=[
        (True),
        (False)
    ]
)
def auto_flush(request):
    return request.param

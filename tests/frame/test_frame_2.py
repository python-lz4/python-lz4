import lz4.frame as lz4frame
import pytest
import os
import sys
from . helpers import roundtrip_chunked

test_data=[
    (b'', 1, 1),
    (os.urandom(8 * 1024), 8, 1),
    (os.urandom(8 * 1024), 1, 8),
    (b'0' * 8 * 1024, 8, 1),
    (b'0' * 8 * 1024, 8, 1),
    (bytearray(b''), 1, 1),
    (bytearray(os.urandom(8 * 1024)), 8, 1),
]
if sys.version_info > (2, 7):
    test_data += [
        (memoryview(b''), 1, 1),
        (memoryview(os.urandom(8 * 1024)), 8, 1)
    ]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param

def test_roundtrip_chunked(data, block_size, block_linked,
                           content_checksum, block_checksum,
                           compression_level,
                           auto_flush, store_size):

    roundtrip_chunked(data, block_size, block_linked,
                      content_checksum, block_checksum,
                      compression_level,
                      auto_flush, store_size)

import lz4.frame as lz4frame
import os
import sys
import pytest
from .helpers import roundtrip_1, roundtrip_2




test_data=[
    (b''),
    (os.urandom(8 * 1024)),
    (b'0' * 8 * 1024),
    (bytearray(b'')),
    (bytearray(os.urandom(8 * 1024))),
]
if sys.version_info > (2, 7):
    test_data += [
        (memoryview(b'')),
        (memoryview(os.urandom(8 * 1024)))
    ]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param


def test_roundtrip_1(data, block_size, block_linked,
                     content_checksum, block_checksum,
                     compression_level, store_size):
    roundtrip_1(data, block_size, block_linked,
                content_checksum, block_checksum,
                compression_level, store_size,
    )

def test_roundtrip_2(data, block_size, block_linked,
                     content_checksum, block_checksum,
                     compression_level, auto_flush,
                     store_size):
    roundtrip_2(data, block_size, block_linked,
                content_checksum, block_checksum,
                compression_level,
                auto_flush, store_size,
    )

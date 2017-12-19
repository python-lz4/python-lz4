import lz4.frame as lz4frame
import pytest
import os
from . helpers import get_frame_info_1

test_data=[
    # (b''),
    # (os.urandom(8 * 1024)),
    (os.urandom(512 * 1024)),
    # (b'0' * 8 * 1024),
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


def test_get_frame_info_1(data, block_size, block_linked,
                          content_checksum, compression_level,
                          store_size):
    get_frame_info_1(data, block_size, block_linked,
                     content_checksum, compression_level,
                     store_size
    )


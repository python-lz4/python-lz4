import lz4.frame as lz4frame
import pytest
import os
from . helpers import roundtrip_1, roundtrip_2

test_data=[
    (os.urandom(128 * 1024)),
    (os.urandom(256 * 1024)),
    (os.urandom(512 * 1024)),
    (os.urandom(1024 * 1024)),
]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param


def test_roundtrip_1(data, block_size):
    roundtrip_1(data, block_size)


def test_roundtrip_2(data, block_size):
    roundtrip_2(data, block_size)



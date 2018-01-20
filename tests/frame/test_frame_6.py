import os
import pytest
import lz4.frame as lz4frame

test_data=[
    #b'',
    (128 * (32 * os.urandom(32))),
    # (256 * (32 * os.urandom(32))),
    # (512 * (32 * os.urandom(32))),
    # (1024 * (32 * os.urandom(32))),
]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param


def test_lz4frame_open_write(data):
    with lz4frame.open('testfile', mode='wb') as fp:
        fp.write(data)

def test_lz4frame_open_write_read(data):
    with lz4frame.open('testfile', mode='wb') as fp:
        fp.write(data)
    with lz4frame.open('testfile', mode='r') as fp:
        data_out = fp.read()
    assert data_out == data

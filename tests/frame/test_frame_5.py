import lz4.frame
import time

def test_frame_decompress_mem_usage():

    try:
        import tracemalloc # Python >= 3.4 only
    except:
        return 0

    tracemalloc.start()

    input_data = 'a' * 1024 * 1024
    compressed = lz4.frame.compress(input_data.encode('utf-8'))
    prev_snapshot = None

    for i in range(5000):
        decompressed = lz4.frame.decompress(compressed)

        if i % 100 == 0:
            snapshot = tracemalloc.take_snapshot()

            if prev_snapshot:
                stats = snapshot.compare_to(prev_snapshot, 'lineno')
                if stats[0].size_diff > 1024 * 4:
                    return 1

            prev_snapshot = snapshot



# TODO: add many more memory usage tests along the lines of this one for other funcs
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


import lz4.frame as laz4frame


def test_lz4frame_open_write(data):
    with lz4frame.open('testfile', mode='wb') as fp:
        fp.write(data)

def test_lz4frame_open_write_read(data):
    with lz4frame.open('testfile', mode='wb') as fp:
        fp.write(data)
    with lz4frame.open('testfile', mode='r') as fp:
        data_out = fp.read()
    assert data_out == data

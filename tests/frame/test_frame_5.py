import lz4.frame
import time
import pytest
tracemalloc = pytest.importorskip('tracemalloc')


test_data=[
    (b'a' * 1024 * 1024),
]

@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param


def test_frame_decompress_mem_usage(data):
    tracemalloc.start()

    compressed = lz4.frame.compress(data)
    prev_snapshot = None

    for i in range(1000):
        decompressed = lz4.frame.decompress(compressed)

        if i % 100 == 0:
            snapshot = tracemalloc.take_snapshot()

            if prev_snapshot:
                stats = snapshot.compare_to(prev_snapshot, 'lineno')
                assert stats[0].size_diff < (1024 * 4)

            prev_snapshot = snapshot


def test_frame_decompress_chunk_mem_usage(data):
    tracemalloc.start()

    compressed = lz4.frame.compress(data)

    prev_snapshot = None

    for i in range(1000):
        context = lz4.frame.create_decompression_context()
        decompressed = lz4.frame.decompress_chunk(context, compressed)

        if i % 100 == 0:
            snapshot = tracemalloc.take_snapshot()

            if prev_snapshot:
                stats = snapshot.compare_to(prev_snapshot, 'lineno')
                assert stats[0].size_diff < (1024 * 10)

            prev_snapshot = snapshot


def test_frame_open_decompress_mem_usage(data):
    tracemalloc.start()

    with lz4.frame.open('test.lz4', 'w') as f:
        f.write(data)

    prev_snapshot = None

    for i in range(1000):
        with lz4.frame.open('test.lz4', 'r') as f:
            decompressed = f.read()

        if i % 100 == 0:
            snapshot = tracemalloc.take_snapshot()

            if prev_snapshot:
                stats = snapshot.compare_to(prev_snapshot, 'lineno')
                assert stats[0].size_diff < (1024 * 10)

            prev_snapshot = snapshot

# TODO: add many more memory usage tests along the lines of this one for other funcs

import lz4.stream
import pytest
import sys
import os
import psutil
import gc


_1KB = 1024
_1MB = _1KB * 1024
_1GB = _1MB * 1024


def run_gc_param_data_buffer_size(func):
    if os.environ.get('TRAVIS') is not None or os.environ.get('APPVEYOR') is not None:
        def wrapper(data, buffer_size, *args, **kwargs):
            return func(data, buffer_size, *args, **kwargs)
    else:
        def wrapper(data, buffer_size, *args, **kwargs):
            gc.collect()
            try:
                result = func(data, buffer_size, *args, **kwargs)
            finally:
                gc.collect()
            return result

    wrapper.__name__ = func.__name__
    return wrapper


def compress(x, c_kwargs):
    if c_kwargs.get('return_bytearray', False):
        c = bytearray()
    else:
        c = bytes()
    with lz4.stream.LZ4StreamCompressor(**c_kwargs) as proc:
        for start in range(0, len(x), c_kwargs['buffer_size']):
            chunk = x[start:start + c_kwargs['buffer_size']]
            block = proc.compress(chunk)
            c += block
    return c


def decompress(x, d_kwargs):
    if d_kwargs.get('return_bytearray', False):
        d = bytearray()
    else:
        d = bytes()
    with lz4.stream.LZ4StreamDecompressor(**d_kwargs) as proc:
        start = 0
        while start < len(x):
            block = proc.get_block(x[start:])
            chunk = proc.decompress(block)
            d += chunk
            start += d_kwargs['store_comp_size'] + len(block)
    return d


test_buffer_size = sorted(
    [256,
     1 * _1KB,
     64 * _1KB,
     1 * _1MB,
     1 * _1GB,
     lz4.stream.LZ4_MAX_INPUT_SIZE]
)


@pytest.fixture(
    params=test_buffer_size,
    ids=[
        'buffer_size' + str(i) for i in range(len(test_buffer_size))
    ]
)
def buffer_size(request):
    return request.param


test_data = [
    (b'a' * _1MB),
]


@pytest.fixture(
    params=test_data,
    ids=[
        'data' + str(i) for i in range(len(test_data))
    ]
)
def data(request):
    return request.param


@run_gc_param_data_buffer_size
def test_block_decompress_mem_usage(data, buffer_size):
    kwargs = {
        'strategy': "double_buffer",
        'buffer_size': buffer_size,
        'store_comp_size': 4,
    }

    if os.environ.get('TRAVIS') is not None:
        pytest.skip('Skipping test on Travis due to insufficient memory')

    if os.environ.get('APPVEYOR') is not None:
        pytest.skip('Skipping test on AppVeyor due to insufficient resources')

    if sys.maxsize < 0xffffffff:
        pytest.skip('Py_ssize_t too small for this test')

    if psutil.virtual_memory().available < 3 * kwargs['buffer_size']:
        # The internal LZ4 context will request at least 3 times buffer_size
        # as memory (2 buffer_size for the double-buffer, and 1.x buffer_size
        # for the output buffer)
        pytest.skip('Insufficient system memory for this test')

    tracemalloc = pytest.importorskip('tracemalloc')

    # Trace memory usage on compression
    tracemalloc.start()
    prev_snapshot = None

    for i in range(1000):
        compressed = compress(data, kwargs)

        if i % 100 == 0:
            snapshot = tracemalloc.take_snapshot()

            if prev_snapshot:
                # Filter on lz4.stream module'a allocations
                stats = [x for x in snapshot.compare_to(prev_snapshot, 'lineno')
                         if lz4.stream.__file__ in x.traceback._frames[0][0]]
                assert sum(map(lambda x: x.size_diff, stats)) < (1024 * 4)

            prev_snapshot = snapshot

    tracemalloc.stop()

    tracemalloc.start()
    prev_snapshot = None

    for i in range(1000):
        decompressed = decompress(compressed, kwargs)  # noqa: F841

        if i % 100 == 0:
            snapshot = tracemalloc.take_snapshot()

            if prev_snapshot:
                # Filter on lz4.stream module'a allocations
                stats = [x for x in snapshot.compare_to(prev_snapshot, 'lineno')
                         if lz4.stream.__file__ in x.traceback._frames[0][0]]
                assert sum(map(lambda x: x.size_diff, stats)) < (1024 * 4)

            prev_snapshot = snapshot

    tracemalloc.stop()

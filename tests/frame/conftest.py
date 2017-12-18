import pytest
# import random
import lz4.frame as lz4frame

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

compression_levels = list(range(-5, 13)) + [
        lz4frame.COMPRESSIONLEVEL_MIN,
        lz4frame.COMPRESSIONLEVEL_MINHC,
        lz4frame.COMPRESSIONLEVEL_MAX,
    ]
compression_levels = [
    # Although testing with all compression levels is desirable, the number of
    # tests becomes too large. So, we'll select some compression levels at
    # random.
    # (i) for i in random.sample(set(compression_levels), k=2)
    (i) for i in compression_levels
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

@pytest.fixture(
    params=[
        (True),
        (False)
    ]
)
def store_size(request):
    return request.param

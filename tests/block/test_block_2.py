import pytest
import sys
import lz4.block
import psutil
import os

_4GB = 0x100000000  # 4GB


@pytest.mark.skipif(os.environ.get('TRAVIS') is not None,
                    'Skippin test on Travis due to insufficient memory')
@pytest.mark.skipif(sys.maxsize < 0xffffffff,
                    reason='Py_ssize_t too small for this test')
@pytest.mark.skipif(psutil.virtual_memory().total < _4GB,
                    reason='Insufficient system memory for this test')
def test_huge():
    try:
        huge = b'\0' * _4GB
    except MemoryError:
        pytest.skip('Insufficient system memory for this test')
        
    with pytest.raises(
            OverflowError, match='Input too large for LZ4 API'
    ):
        lz4.block.compress(huge)

    with pytest.raises(
            OverflowError, match='Dictionary too large for LZ4 API'
    ):
        lz4.block.compress(b'', dict=huge)

    with pytest.raises(
            OverflowError, match='Input too large for LZ4 API'
    ):
        lz4.block.decompress(huge)

    with pytest.raises(
            OverflowError, match='Dictionary too large for LZ4 API'
    ):
            lz4.block.decompress(b'', dict=huge)

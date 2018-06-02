import pytest
import sys
#import lz4.block

@pytest.mark.skipif(sys.maxsize < 0xffffffff,
                    reason='Py_ssize_t too small for this test')
def test_huge():
    try:
        huge = b'\0' * 0x100000000  # warning: this allocates 4GB of memory!
    except MemoryError:
        pytest.skip('Insufficient system memory for this test')

    # with pytest.raises(
    #         OverflowError, match='Input too large for LZ4 API'
    # ):
    #     lz4.block.compress(huge)

    # with pytest.raises(
    #         OverflowError, match='Dictionary too large for LZ4 API'
    # ):
    #     lz4.block.compress(b'', dict=huge)

    # with pytest.raises(
    #         OverflowError, match='Input too large for LZ4 API'
    # ):
    #     lz4.block.decompress(huge)

    # with pytest.raises(
    #         OverflowError, match='Dictionary too large for LZ4 API'
    # ):
    #         lz4.block.decompress(b'', dict=huge)

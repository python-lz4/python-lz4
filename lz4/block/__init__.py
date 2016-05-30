from ._block import compress, decompress

# The following definitions are for backwards compatibility, and will
# be removed in the future
import warnings

def LZ4_compress(source):
    warnings.warn('This function is deprecated. Use lz4.block.compress instead.', 
                 DeprecationWarning)
    return compress(source)

def dumps(source):
    warnings.warn('This function is deprecated. Use lz4.block.compress instead.', 
                 DeprecationWarning)
    return compress(source)

def LZ4_compress_fast(source):
    warnings.warn('This function is deprecated. Use lz4.block.compress with mode=\'fast\' instead.', 
                 DeprecationWarning)
    return compress(source, mode='fast')

def compress_fast(source):
    warnings.warn('This function is deprecated. Use lz4.block.compress with mode=\'fast\' instead.', 
                 DeprecationWarning)
    return compress(source, mode='fast')

def compressHC(source):
    warnings.warn('This function is deprecated. Use lz4.block.compress with mode=\'high_compression\' instead.', 
                 DeprecationWarning)
    return compress(source, mode='high_compression')

def uncompress(source):
    warnings.warn('This function is deprecated. Use lz4.block.decompress instead.', 
                 DeprecationWarning)
    return decompress(source)

def LZ4_uncompress(source):
    warnings.warn('This function is deprecated. Use lz4.block.decompress instead.', 
                 DeprecationWarning)
    return decompress(source)

def loads(source):
    warnings.warn('This function is deprecated. Use lz4.block.decompress instead.', 
                 DeprecationWarning)
    return decompress(source)

# Package version info, generated on install
from .version import version as __version__
VERSION = __version__

try:
    from .lz4version import LZ4_VERSION
except ImportError:
    LZ4_VERSION = None

from ._version import lz4version

# The following definitions are for backwards compatibility, and will
# be removed in the future
import lz4.block
from .deprecated import deprecated

@deprecated('use lz4.block.compress instead')
def compress(*args, **kwargs):
    return lz4.block.compress(*args, **kwargs)

@deprecated('use lz4.block.decompress instead')
def decompress(*args, **kwargs):
    return lz4.block.decompress(*args, **kwargs)

@deprecated('use lz4.block.compress instead')
def LZ4_compress(source):
    return lz4.block.compress(source)

@deprecated('use lz4.block.compress instead')
def dumps(source):
    return lz4.block.compress(source)

@deprecated('use lz4.block.compress instead')
def LZ4_compress_fast(source):
    return lz4.block.compress(source, mode='fast')

@deprecated('use lz4.block.compress instead')
def compress_fast(source):
    return lz4.block.compress(source, mode='fast')

@deprecated('use lz4.block.compress instead')
def compressHC(source):
    return lz4.block.compress(source, mode='high_compression')

@deprecated('use lz4.block.decompress instead')
def uncompress(source):
    return lz4.block.decompress(source)

@deprecated('use lz4.block.decompress instead')
def LZ4_uncompress(source):
    return lz4.block.decompress(source)

@deprecated('use lz4.block.decompress instead')
def loads(source):
    return lz4.block.decompress(source)

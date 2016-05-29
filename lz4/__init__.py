from .block import loads, dumps

# Package version info, generated on install
from .version import version as __version__
VERSION = __version__

try:
    from .lz4version import LZ4_VERSION
except ImportError:
    LZ4_VERSION = None

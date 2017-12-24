# Package version info, generated on install
from .version import version as __version__
VERSION = __version__

from ._version import (
    library_version_number,
    library_version_string,
)

from deprecation import deprecated

@deprecated(deprecated_in="0.14", removed_in="1.0",
            # See https://github.com/briancurtin/deprecation/issues/9
            # current_version=__version__,
            details="Use the lz4.library_version_number function instead")
def lz4version():
    return library_version_number()

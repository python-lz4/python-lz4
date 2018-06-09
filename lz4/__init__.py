from pkg_resources import get_distribution, DistributionNotFound
try:
    __version__ = get_distribution(__name__).version
except DistributionNotFound:
    # package is not installed
    pass

VERSION = __version__


from ._version import (  # noqa: F401
    library_version_number,
    library_version_string,
)

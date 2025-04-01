from typing import Union, Optional

BufferProtocol = Union[bytes, bytearray, memoryview]


def compress(
        source: Union[str, BufferProtocol],
        mode: str = 'default',
        store_size: bool = True,
        acceleration: int = 1,
        compression: int = 9,
        return_bytearray: bool = False,
        dict: Optional[bytes] = None
) -> Union[bytes, bytearray]:
    ...


def decompress(
        source: Union[str, BufferProtocol],
        uncompressed_size: int = -1,
        return_bytearray: bool = False,
        dict: Union[str, bytes] = None
) -> Union[bytes, bytearray]:
    ...


class LZ4BlockError(Exception):
    """Call to LZ4 library failed."""
    ...

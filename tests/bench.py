import uuid
import timeit
import lz4
import os
from timeit import Timer
import sys
import blosc

DATA = open(sys.argv[1], "rb").read()
LZ4_DATA = lz4.block.compress(DATA)
BLOSC_DATA = blosc.compress(DATA, cname='lz4', clevel=5, shuffle=True)
LOOPS = 100

print("Data Size:")
print("  Input: %d" % len(DATA))
print("  LZ4: %d (%.2f)" % (len(LZ4_DATA), len(LZ4_DATA) / float(len(DATA))))
print("  Blosc: %d (%.2f)" % (len(BLOSC_DATA), len(BLOSC_DATA) / float(len(DATA))))
print("  LZ4 / Blosc: %f" % (float(len(LZ4_DATA)) / float(len(BLOSC_DATA))))

print("Benchmark: %d calls" % LOOPS)
print("  LZ4 Compression: %fs" % (Timer("lz4.block.compress(DATA)", "from __main__ import DATA; import lz4").timeit(number=LOOPS)/LOOPS))
print("  Blosc Compression: %fs" % (Timer("blosc.compress(DATA, cname='lz4', clevel=5, shuffle=True)", "from __main__ import DATA; import blosc").timeit(number=LOOPS)/LOOPS))
print("  LZ4 Decompression: %fs" % (Timer("lz4.block.decompress(LZ4_DATA)", "from __main__ import LZ4_DATA; import lz4").timeit(number=LOOPS)/LOOPS))
print("  Blosc Decompression : %fs" % (Timer("blosc.decompress(BLOSC_DATA)", "from __main__ import BLOSC_DATA; import blosc").timeit(number=LOOPS)/LOOPS))

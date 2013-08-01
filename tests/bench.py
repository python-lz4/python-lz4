import uuid
import timeit
import lz4
import snappy
import os
from timeit import Timer

DATA = open("../src/lz4.c", "rb").read()
LZ4_DATA = lz4.compress(DATA)
SNAPPY_DATA = snappy.compress(DATA)
LOOPS = 200000

print("Data Size:")
print("  Input: %d" % len(DATA))
print("  LZ4: %d (%.2f)" % (len(LZ4_DATA), len(LZ4_DATA) / float(len(DATA))))
print("  Snappy: %d (%.2f)" % (len(SNAPPY_DATA), len(SNAPPY_DATA) / float(len(DATA))))
print("  LZ4 / Snappy: %f" % (float(len(LZ4_DATA)) / float(len(SNAPPY_DATA))))

print("Benchmark: %d calls" % LOOPS)
print("  LZ4 Compression: %fs" % Timer("lz4.compress(DATA)", "from __main__ import DATA; import lz4").timeit(number=LOOPS))
print("  Snappy Compression: %fs" % Timer("snappy.compress(DATA)", "from __main__ import DATA; import snappy").timeit(number=LOOPS))
print("  LZ4 Decompression: %fs" % Timer("lz4.uncompress(LZ4_DATA)", "from __main__ import LZ4_DATA; import lz4").timeit(number=LOOPS))
print("  Snappy Decompression : %fs" % Timer("snappy.uncompress(SNAPPY_DATA)", "from __main__ import SNAPPY_DATA; import snappy").timeit(number=LOOPS))

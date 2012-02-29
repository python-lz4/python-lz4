import uuid
import timeit
import lz4
import snappy
from timeit import Timer

DATA = open("/dev/urandom", "rb").read(128 * 1024)  # Read 128kb
LZ4_DATA = lz4.compress(DATA)
SNAPPY_DATA = snappy.compress(DATA)
LOOPS = 200000

print "Data Size:"
print "  Input: %d" % len(DATA)
print "  LZ4: %d" % len(LZ4_DATA)
print "  Snappy: %d" % len(SNAPPY_DATA)
print "  LZ4 / Snappy: %f" % (float(len(LZ4_DATA)) / float(len(SNAPPY_DATA)))

print "Benchmark: %d calls" % LOOPS
print "  LZ4 Compression: %fs" % Timer("lz4.compress(DATA)", "from __main__ import DATA; import lz4").timeit(number=LOOPS)
print "  Snappy Compression: %fs" % Timer("snappy.compress(DATA)", "from __main__ import DATA; import snappy").timeit(number=LOOPS)
print "  LZ4 Decompression: %fs" % Timer("lz4.uncompress(LZ4_DATA)", "from __main__ import LZ4_DATA; import lz4").timeit(number=LOOPS)
print "  Snappy Decompression : %fs" % Timer("snappy.uncompress(SNAPPY_DATA)", "from __main__ import SNAPPY_DATA; import snappy").timeit(number=LOOPS)

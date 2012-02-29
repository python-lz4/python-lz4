import lz4
import sys

DATA = open("/dev/urandom", "rb").read(128 * 1024)  # Read 128kb
sys.exit(DATA != lz4.loads(lz4.dumps(DATA)) and 1 or 0)

import lz4.frame
import time

def test_frame_decompress_mem_usage():

    try:
        import tracemalloc # Python >= 3.4 only
    except:
        return 0

    tracemalloc.start()

    input_data = 'a' * 1024 * 1024
    compressed = lz4.frame.compress(input_data.encode('utf-8'))
    prev_snapshot = None

    for i in range(5000):
        decompressed = lz4.frame.decompress(compressed)

        if i % 100 == 0:
            snapshot = tracemalloc.take_snapshot()

            if prev_snapshot:
                stats = snapshot.compare_to(prev_snapshot, 'lineno')
                if stats[0].size_diff > 1024 * 4:
                    return 1

            prev_snapshot = snapshot



# TODO: add many more memory usage tests along the lines of this one for other funcs

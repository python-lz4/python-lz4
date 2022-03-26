import lz4.frame as lz4frame


def test_lz4frame_open_write_read_text_iter():
    data_1 = b"This is a..."
    data_2 = b" test string!"

    with lz4frame.open("testfile", mode="w") as fp_write:
        fp_write.write(data_1)
        fp_write.flush()

        fp_write.write(data_2)

        with lz4frame.open("testfile", mode="r") as fp_read:
            assert fp_read.read() == data_1

        fp_write.flush()

        with lz4frame.open("testfile", mode="r") as fp_read:
            assert fp_read.read() == data_1 + data_2

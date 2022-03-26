import lz4.frame as lz4frame


def test_lz4frame_open_write_read_text_iter():
    data_1 = u"This is a..."
    data_2 = u"...test string!"

    fp = lz4frame.open("testfile", mode="wt")
    fp.write(data_1)
    fp.flush()
    # fp.flush()

    # fp.write(data_2)

    with lz4frame.open("testfile", mode="rt") as fp_read:
        print(fp_read.read())
        assert fp_read.read() == data_1

    fp.flush()

    with lz4frame.open("testfile", mode="rt") as fp_read:
        assert fp_read.read() == data_1 + data_2

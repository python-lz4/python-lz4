==========
python-lz4
==========

Overview
========
This package provides bindings for the `lz4 compression library <http://code.google.com/p/lz4/>`_ by Yann Collet.

Usage
=====
The library is pretty simple to use::

    >>> import lz4
    >>> compressed_data = lz4.dumps(data)
    >>> data == lz4.loads(compressed_data)
    True
    >>>

Methods
=======
The bindings provides some aliases too:

    >>> import lz4
    >>> lz4.LZ4_compress == lz4.compress == lz4.dumps
    True
    >>> lz4.LZ4_uncompress == lz4.uncompress == lz4.loads
    True
    >>>

Is it fast ?
============
Yes. Here are the results on my 2011 Macbook Pro i7: ::

    200000 calls - LZ4:
      Best: 3.418643 seconds
      Worst: 3.473787 seconds
    200000 calls - Snappy:
      Best: 4.203340 seconds
      Worst: 4.272133 seconds

With the following code: ::

    >>> import uuid
    >>> import timeit
    >>> from timeit import Timer
    >>> DATA = "".join([ str(uuid.uuid4()) for _ in xrange(200)])
    >>> LOOPS = 200000
    >>> times = [Timer("lz4.dumps(DATA)", "from __main__ import DATA; import lz4").timeit(number=LOOPS) for x in xrange(10)]
    >>> print "%d calls - LZ4:" % LOOPS
    >>> print "  Best: %f seconds" % min(times)
    >>> print "  Worst: %f seconds" % max(times)
    >>> times = [Timer("snappy.compress(DATA)", "from __main__ import DATA; import snappy").timeit(number=LOOPS) for x in xrange(10)]
    >>> print "%d calls - Snappy:" % LOOPS
    >>> print "  Best: %f seconds" % min(times)
    >>> print "  Worst: %f seconds" % max(times)

Important note
==============
Because LZ4 doesn't define a container format, the python bindings will insert the original data size as an integer at the start of the compressed payload, like most bindings do anyway (Java...)

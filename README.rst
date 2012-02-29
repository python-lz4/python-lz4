==========
python-lz4 
========== 

.. image:: https://secure.travis-ci.org/steeve/python-lz4.png?branch=master

Overview
========
This package provides bindings for the `lz4 compression library <http://code.google.com/p/lz4/>`_ by Yann Collet.

Install
=======
The package is hosted on `PyPI <http://pypi.python.org/pypi/lz4>`_::

    $ pip install lz4
    $ easy_install lz4

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

    Compression:
    200000 calls - LZ4:
      Best: 2.918570 seconds
      Worst: 2.966427 seconds
    200000 calls - Snappy:
      Best: 3.634658 seconds
      Worst: 3.670079 seconds
    Decompression
    200000 calls - LZ4:
      Best: 0.458944 seconds
      Worst: 0.483467 seconds
    200000 calls - Snappy:
      Best: 0.714303 seconds
      Worst: 0.753677 seconds

With the following code: ::

    >>> import uuid
    >>> import timeit
    >>> import lz4
    >>> import snappy
    >>> from timeit import Timer

    >>> DATA = "".join([ str(uuid.uuid4()) for _ in xrange(200)])
    >>> LZ4_DATA = lz4.compress(DATA)
    >>> SNAPPY_DATA = snappy.compress(DATA)
    >>> LOOPS = 200000

    >>> print "Compression:"
    >>> times = [Timer("lz4.compress(DATA)", "from __main__ import DATA; import lz4").timeit(number=LOOPS) for x in xrange(10)]
    >>> print "%d calls - LZ4:" % LOOPS
    >>> print "  Best: %f seconds" % min(times)
    >>> print "  Worst: %f seconds" % max(times)
    >>> times = [Timer("snappy.compress(DATA)", "from __main__ import DATA; import snappy").timeit(number=LOOPS) for x in xrange(10)]
    >>> print "%d calls - Snappy:" % LOOPS
    >>> print "  Best: %f seconds" % min(times)
    >>> print "  Worst: %f seconds" % max(times)

    >>> print "Decompression"
    >>> times = [Timer("lz4.uncompress(LZ4_DATA)", "from __main__ import LZ4_DATA; import lz4").timeit(number=LOOPS) for x in xrange(10)]
    >>> print "%d calls - LZ4:" % LOOPS
    >>> print "  Best: %f seconds" % min(times)
    >>> print "  Worst: %f seconds" % max(times)
    >>> times = [Timer("snappy.uncompress(SNAPPY_DATA)", "from __main__ import SNAPPY_DATA; import snappy").timeit(number=LOOPS) for x in xrange(10)]
    >>> print "%d calls - Snappy:" % LOOPS
    >>> print "  Best: %f seconds" % min(times)
    >>> print "  Worst: %f seconds" % max(times)

Important note
==============
Because LZ4 doesn't define a container format, the python bindings will insert the original data size as an integer at the start of the compressed payload, like most bindings do anyway (Java...)

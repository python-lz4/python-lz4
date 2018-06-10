Install
=======

The package is hosted on `PyPI <https://pypi.org/project/lz4/>`_ and so can be
installed via pip::

  $ pip install lz4

The LZ4 bindings require linking to the LZ4 library, and so if there is not a
pre-compiled wheel available for your platform you will need to have a suitable
C compiler available, as well as the Python development header files.

The LZ4 library bindings provided by this package require the LZ4 library. If
the system already has an LZ4 library and development header files present, and
the library is a recent enough version the package will build against that.
Otherwise, the package will use a bundled version of the library files to link
against. The package currently requires LZ4 version 1.7.5 or later.

The package can also be installed manually::

  $ python setup.py install

Several packages need to be present on the system ahead of running this command.
They can be installed using ``pip``::

  $ pip install -r requirements.txt

Test suite
----------

The package includes an extensive test suite that can be run using::

  $ python setup.py test

or, preferably, via ``tox``::

  $ tox

Documentation
-------------

The package also includes documentation in the ``docs`` directory. The
documentation is built using `Sphinx <http://www.sphinx-doc.org/en/stable/>`_,
and can be built using the included ``Makefile``::

  $ cd docs
  $ make html

To see other documentation targets that are available use the command ``make help``.

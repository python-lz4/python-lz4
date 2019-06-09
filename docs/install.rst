Install
=======

The bindings to the LZ4 compression library provided by this package are in the
form of a Python extension module written in C. These extension modules need to
be compiled against the LZ4 library and the Python

Installing from pre-built wheels
--------------------------------

The package is hosted on `PyPI <https://pypi.org/project/lz4/>`_ and pre-built
wheels are available for Linux, OSX and Windows. Installation using a pre-built
wheel can be achieved by::

  $ pip install lz4


Installing from source
----------------------

The LZ4 bindings require linking to the LZ4 library, and so if there is not a
pre-compiled wheel available for your platform you will need to have a suitable
C compiler available, as well as the Python development header files. On
Debian/Ubuntu based systems the header files for Python are found in the
distribution package ``pythonX.Y-dev`` e.g. ``python3.7-dev``. On Fedora/Red Hat
based systems, the Python header files are found in the distribution package
``python-devel``.

The LZ4 library bindings provided by this package require the LZ4 library. If
the system already has an LZ4 library and development header files present, and
the library is a recent enough version the package will build against that.
Otherwise, the package will use a bundled version of the library files to link
against. The package currently requires LZ4 version 1.7.5 or later. 

On a system for which there are no pre-built wheels available on PyPi, running
this command will result in the extension modules being compiled from source::

  $ pip install lz4

On systems for which pre-built wheels are available, the following command will
force a local compilation of the extension modules from source::

  $ pip install --no-binary --no-cache-dir lz4

The package can also be installed manually from a checkout of the source code
git repository::

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

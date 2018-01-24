==========
python-lz4
==========

Status
======

.. image:: https://travis-ci.org/python-lz4/python-lz4.svg?branch=master
   :target: https://travis-ci.org/python-lz4/python-lz4
   :alt: Build Status

.. image:: https://ci.appveyor.com/api/projects/status/github/python-lz4/python-lz4?branch=master
   :target: https://ci.appveyor.com/project/jonathanunderwood/python-lz4
   :alt: Build Status Windows

.. image:: https://readthedocs.org/projects/python-lz4/badge/?version=latest
   :target: https://readthedocs.org/projects/python-lz4/
   :alt: Documentation

Introduction
============
This package provides python bindings for the `LZ4 compression library
<https://lz4.github.io/lz4/>`_.

The bindings provided in this package cover the `frame format
<http://lz4.github.io/lz4/lz4_Frame_format.html>`_ and the `block format
<http://lz4.github.io/lz4/lz4_Block_format.html>`_ specifications. The frame
format bindings are the recommended ones to use, as this guarantees
interoperability with other implementations and language bindings.

A future release may implement support for the LZ4 stream format. Patches and
help are welcome.

Documenation
============

.. image:: https://readthedocs.org/projects/python-lz4/badge/?version=latest
   :target: https://readthedocs.org/projects/python-lz4/
   :alt: Documentation

Full documentation is included with the project. The documentation is
generated using Sphinx. Documentation is also hosted on readthedocs.

:master: http://python-lz4.readthedocs.io/en/stable/
:development: http://python-lz4.readthedocs.io/en/latest/
   
Licensing
=========
Code specific to this project is covered by the `BSD 3-Clause License
<http://opensource.org/licenses/BSD-3-Clause>`_


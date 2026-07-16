Licensing
=========
Code specific to this project is covered by the `BSD 3-Clause License
<http://opensource.org/licenses/BSD-3-Clause>`_

This package bundles the LZ4 and xxHash C libraries (under ``lz4libs/``) when
they are built from source rather than linked against a system-provided
``liblz4``. Those libraries are covered by their own `BSD 2-Clause License
<http://opensource.org/licenses/BSD-2-Clause>`_, copyright Yann Collet. The
license texts are included alongside the vendored sources in
``lz4libs/LICENSE`` and ``lz4libs/LICENSE.xxhash``, and are packaged with the
distributed sdist and wheel.

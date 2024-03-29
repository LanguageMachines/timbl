[![GitHub build](https://github.com/LanguageMachines/timbl/actions/workflows/timbl.yml/badge.svg?branch=master)](https://github.com/LanguageMachines/timbl/actions/)
[![Language Machines Badge](http://applejack.science.ru.nl/lamabadge.php/timbl)](http://applejack.science.ru.nl/languagemachines/)
[![DOI](https://zenodo.org/badge/20526237.svg)](https://zenodo.org/badge/latestdoi/20526237)

===========================================
TiMBL: Tilburg Memory Based Learner
===========================================

    TiMBL 6.4 (c) CLS/ILK/CLiPS 1998 - 2024
    Centre for Language Studies, Radboud University Nijmegen
    Induction of Linguistic Knowledge Research Group, Tilburg University and
    Centre for Dutch Language and Speech, University of Antwerp

**Website:** https://languagemachines.github.io/timbl/


TiMBL is an open source software package implementing several memory-based
learning algorithms, among which IB1-IG, an implementation of k-nearest
neighbor classification with feature weighting suitable for symbolic feature
spaces, and IGTree, a decision-tree approximation of IB1-IG. All implemented
algorithms have in common that they store some representation of the training
set explicitly in memory. During testing, new cases are classified by
extrapolation from the most similar stored cases.

For over fifteen years TiMBL has been mostly used in natural language
processing as a machine learning classifier component, but its use extends to
virtually any supervised machine learning domain. Due to its particular
decision-tree-based implementation, TiMBL is in many cases far more efficient
in classification than a standard k-nearest neighbor algorithm would be.


-----------------------------------------------------------------------

This is a major extension to the sixth main release of TiMBL.
Most significant change: **The main program is now called 'timbl' and not
'Timbl' anymore. Be warned!**
This change is part of our effort to get our MBL software into software
distributions like Debian, Ubuntu, RedHat .

Comments and bug-reports are welcome at our issue tracker at
https://github.com/LanguageMachines/timbl/issues or by mailing
lamasoftware (at) science.ru.nl.
Documentation and more info may be found on https://languagemachines.github.io/timbl .

TiMBL is distributed under the GNU Public Licence v3 (see the file COPYING).

-----------------------------------------------------------------------

This software has been tested on:
- Intel platforms running several versions of Linux, including Ubuntu, Debian,
  Arch Linux, Fedora (both 32 and 64 bits)
- MAC platform running OS X 10.10

Alternatively, with some effort, you may get it to work on a Windows platform using Cygwin.

Compilers:
 - GCC (use 7.0 or later)
 - Clang

Contents of this distribution:
- Sources
- Licensing information ( COPYING )
- Build system based on GNU Autotools
- Container build file ( Dockerfile )
- Example data files ( in the demos directory )
- Documentation ( in the docs directory )

Dependencies:
To be able to succesfully build TiMBL from the tarball, you need the
following pakages:
- ticcutils (https://github.com/LanguageMachines/ticcutils)
- pkg-config
- libxml2-dev

To install TiMBL, first consult whether your distribution's package manager has an up-to-date package for TiMBL.

To compile and install manually from source instead, provided you have all the dependencies installed:

    $ bash bootstrap.sh
    $ ./configure
    $ make
    $ make install

If you want to automatically download and install the latest stable versions of
the required dependencies, then run `./build-deps.sh` prior to the above. You
can pass a target directory prefix as first argument and you may need to
prepend `sudo` to ensure you can install there. The dependencies are:

* [ticcutils](https://github.com/LanguageMachines/ticcutils)

A `Dockerfile` for a container build is also available, specify `--build-arg VERSION=development` if you want the latest
development version instead.

You will still need to take care to install the following 3rd party
dependencies through your distribution's package manager, as they are not
provided by our script:

* ``icu`` - A C++ library for Unicode and Globalization support. On Debian/Ubuntu systems, install the package libicu-dev.
* A sane build environment with a C++ compiler (e.g. gcc 4.9 or above or clang), make, autotools, libtool, pkg-config


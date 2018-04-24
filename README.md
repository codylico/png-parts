# PNG Parts: parts of a Portable Network Graphics implementation

The PNG Parts repository aims to provide a working implementation of
the Portable Network Graphics image format as specified in RFC 2083.
The parts of the implementation, to be provided in both C and C++,
should be more or less usable independently of each other.

## Goals

Right now, this repository is empty. The goals of this project include:

* DEFLATE compression implementation (both read and write)
  according to [RFC 1951](https://www.ietf.org/rfc/rfc1951.txt)

* ZLIB compressed data format implementation (both read and write)
  according to [RFC 1950](https://www.ietf.org/rfc/rfc1950.txt)

* PNG image format basic implementation (both read and write)
  according to [RFC 2083](https://www.ietf.org/rfc/rfc2083.txt)

* support for the tRNS chunk for transparency (both read and write)

* support for additional user-defined chunks (both read and write)

## Build and Installation

This project will use the CMake build system, for easier cross-platform
development. CMake can be found at the following URL:

[https://cmake.org/download/](https://cmake.org/download/)

## License

The source code within this project will be licensed under the
MIT license.

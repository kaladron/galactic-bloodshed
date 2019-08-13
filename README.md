# Galactic Bloodshed

Galactic Bloodshed is one of the oldest games on the Internet and was one of the first 4-X games.

### Prerequisites

GB uses C++20, so a modern compiler and standard library are required.
Bazel must be installed on the system.
Two Boost libraries are required.

Install the following dependancies (may vary based on platform).

#### Ubuntu 19.04

```
sudo apt-get install pkg-config zip g++ zlib1g-dev unzip python
sudo apt-get install g++-9 clang-tidy clang-format
sudo apt-get install libsqlite3-dev
sudo apt-get install libboost-all-dev
# sudo apt-get install google-mock libgtest-dev libgmock-dev cmake

# Not needed if building with autotools
echo "deb [arch=amd64] http://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
curl https://bazel.build/bazel-release.pub.gpg | sudo apt-key add -
sudo apt-get update
sudo apt-get install openjdk-8-jdk
sudo apt-get install bazel
```

## Getting Started

GB is currently hosted on GitHub at https://github.com/kaladron/galactic-bloodshed.  It uses Bazel
for building.

### Checkout sources

```
git clone git@github.com:kaladron/galactic-bloodshed.git
cd galactic-bloodshed
git submodule init
git submodule update
```

### Setup

Doxygen output from the source is at http://doxygen.galacticbloodshed.com/

### Installing

#### Build and Install GUnit and Googletest

```
cd galactic-bloodshet/external/googletest
cmake CMakeLists.txt -Dgtest_disable_pthreads=ON
make
```

You should now see a "lib" folder in `galactic-bloodshet/external/googletest` with four .a files we can add to all test lib dependencies.

#### Using Bazel

cd into the gb directory and run '''bazel build ...'''

#### Using autotools

```
autoreconf --install
mkdir -p build
cd build
../configure
make
sudo make install
```

## Running the tests

cd into the src directory and run '''bazel test ...'''

### And coding style tests

Please run clang-format on any code before submitting a pull request.

## Deployment

TBD

## Contributing

TBD

## Versioning

The version numbers in the doc still reflect the old version numbers.  Haven't figured out what to do here yet.

## Authors

* Jeff Bailey
* Sriram (Sri) Panyam

## License

The original code is under an odd license.  Future code contributions are under Apache2.  We hope to get the original authors to consider relicensing under Apache2 or MIT.


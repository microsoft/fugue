#!/bin/bash

git submodule add https://github.com/microsoft/vcpkg

./vcpkg/bootstrap-vcpkg.sh

export VCPKG_ROOT=$(pwd)/vcpkg

vcpkg/vcpkg install folly
vcpkg/vcpkg install fmt
vcpkg/vcpkg install libuv
vcpkg/vcpkg install double-conversion
vcpkg/vcpkg install boost
vcpkg/vcpkg install rapidjson
vcpkg/vcpkg install protobuf[zlib]
vcpkg/vcpkg install glog
vcpkg/vcpkg install leveldb
vcpkg/vcpkg install snappy

mkdir build && cd build
cmake ..
make


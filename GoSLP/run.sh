#!/usr/bin/bash
set -e

# Check input
if [ $# -ne 2 ]; then
    echo "Usage: $0 <source-file.c> <name of pass (i.e. testPass)>"
    exit 1
fi

testfile="$1"
passName="$2"
tmp="${testfile#*/}"
testfilebase="${tmp%.*}"

# set up and build
mkdir -p build && cd build
cmake -DPASS_DIR="$passName" ..
make

# create IR
clang -emit-llvm -S ../"$testfile" -Xclang -disable-O0-optnone

# run pass
opt -disable-output -load-pass-plugin=./"$passName"/"$passName".so -passes="func-name" "$testfilebase".ll
#!/usr/bin/bash
set -e

# Check input
if [ $# -ne 2 ]; then
    echo "Usage: $0 <source-file.c> <name of pass (i.e. testPass)>"
    exit 1
fi

testfile="$1"
passName="$2"
outFile=
tmp="${testfile#*/}"
testfilebase="${tmp%.*}"

# set up and build
mkdir -p build && cd build
cmake -DPASS_DIR="$passName" ..
make

# create IR
clang -O0 -emit-llvm -S ../"$testfile" -Xclang -disable-O0-optnone -fno-slp-vectorize -fno-discard-value-names

# run pass
opt -load-pass-plugin=./"$passName"/"$passName".so -passes="$passName" -S "$testfilebase".ll -o ./"$passName"/"$testfilebase".ll

# compile to object
clang -c ./"$passName"/"$testfilebase".ll -o ./"$passName"/"$testfilebase".o

# link object -> executable
clang ./"$passName"/"$testfilebase".o -o ./"$passName"/"$testfilebase"_exe

# run it
./"$passName"/"$testfilebase"_exe
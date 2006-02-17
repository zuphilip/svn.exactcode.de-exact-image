#!/bin/sh

CFLAGS="-s -march=athlon64"

build ()
{
        echo '' "$1"
        rm Linux-*/bin/optimize2bw{,.o}
        make CXXFLAGS="${CFLAGS} $1"
        time  Linux-*/bin/optimize2bw -i test.jpg  -o test.tif > /dev/null
        echo
}

CFLAGS="$CFLAGS -O2"

build -fprofile-generate
build -fbranch-probabilities 

build -O2
build "-fomit-frame-pointer -funroll-loops -finline -ftracer -funit-at-a-time"

for x in fgcse fprefetch-loop-arrays ffast-math funsafe-math-optimizations fexpensive-optimizations funit-at-a-time ftracer fpeel-loops frename-registers ftree-vectorize fomit-frame-pointer funroll-loops finline
do
	build -$x
done

for x in O1 O2 O3 Os ; do
	build -$x
done


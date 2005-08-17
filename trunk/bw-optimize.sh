#!/bin/sh

set -x

[ "$1" -a "$2" ] || exit

convert -normalize $1 1-$1
convert -contrast +modulate 110 1-$1 2-$1
convert -unsharp 5x12.5+5+0.1 2-$1 3-$1
convert -threshold 32767 3-$1 $2


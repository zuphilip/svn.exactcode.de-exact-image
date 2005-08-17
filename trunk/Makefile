
all: read-jpeg2 tiff bw-optimize

read-jpeg2: read-jpeg2.c
	gcc -ggdb -std=c99 -o read-jpeg2 read-jpeg2.c -ljpeg

tiff: tiff.c
	gcc -ggdb -std=c99 -ltiff tiff.c -o tiff

bw-optimize: bw-optimize.c
	gcc `Wand-config --cflags --cppflags` bw-optimize.c \
	    `Wand-config --ldflags --libs` -o bw-optimize

# check:
#	display -size 1275x2096 -depth 8 gray:test.raw


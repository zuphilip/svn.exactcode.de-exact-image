
all: read-jpeg2 bw-optimize

read-jpeg2: read-jpeg2.c tiff.o
	gcc -ggdb -std=c99 -o read-jpeg2 tiff.o read-jpeg2.c -ljpeg -ltiff

tiff.o: tiff.c
	gcc -ggdb -std=c99 -c tiff.c -ltiff

bw-optimize: bw-optimize.c
	gcc `Wand-config --cflags --cppflags` bw-optimize.c \
	    `Wand-config --ldflags --libs` -o bw-optimize

# check:
#	display -size 1275x2096 -depth 8 gray:test.raw


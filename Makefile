
all: read-jpeg2 bw-optimize

CFLAGS := -O2 -march=athlon-xp -ggdb

optimize2bw: optimize2bw.c tiff.o read-jpeg2.o
        gcc $(CFLAGS) -std=c99 -o optimize2bw -ljpeg -ltiff

read-jpeg2.o: read-jpeg2.c
	gcc $(CFLAGS) -std=c99 -c read-jpeg2.c

tiff.o: tiff.c
	gcc $(CFLAGS) -std=c99 -c tiff.c

bw-optimize: bw-optimize.c
	gcc `Wand-config --cflags --cppflags` bw-optimize.c \
	    `Wand-config --ldflags --libs` -o bw-optimize

# check:
#	display -size 1275x2096 -depth 8 gray:test.raw


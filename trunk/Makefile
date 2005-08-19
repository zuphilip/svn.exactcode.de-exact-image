
all: optimize2bw empty-page

CFLAGS := -O2 -march=athlon-xp -ggdb

optimize2bw: optimize2bw.c tiff.o jpeg.o
	gcc $(CFLAGS) -std=c99 -o optimize2bw optimize2bw.c tiff.o jpeg.o \
	    -ljpeg -ltiff

empty-page: empty-page.c tiff.o
	gcc $(CFLAGS) -std=c99 -o empty-page empty-page.c tiff.o -ltiff

jpeg.o: jpeg.c
	gcc $(CFLAGS) -std=c99 -c jpeg.c

tiff.o: tiff.c
	gcc $(CFLAGS) -std=c99 -c tiff.c

bw-optimize: bw-optimize.c
	gcc `Wand-config --cflags --cppflags` bw-optimize.c \
	    `Wand-config --ldflags --libs` -o bw-optimize

# check:
#	display -size 1275x2096 -depth 8 gray:test.raw



all: optimize2bw empty-page

CFLAGS := -O2 -ggdb

optimize2bw: optimize2bw.c tiff.o jpeg.o
	g++ $(CFLAGS) -I utility/include -o optimize2bw optimize2bw.c \
	    utility/src/ArgumentList.cc tiff.o jpeg.o -ljpeg -ltiff

empty-page: empty-page.c tiff.o
	g++ $(CFLAGS) -I utility/include -o empty-page empty-page.c \
	    utility/src/ArgumentList.cc tiff.o -ltiff

jpeg.o: jpeg.c
	g++ $(CFLAGS) -c jpeg.c

tiff.o: tiff.c
	g++ $(CFLAGS) -c tiff.c

bw-optimize: bw-optimize.c
	g++ `Wand-config --cflags --cppflags` bw-optimize.c \
	    `Wand-config --ldflags --libs` -o bw-optimize

# check:
#	display -size 1275x2096 -depth 8 gray:test.raw


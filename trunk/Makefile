
all: read-jpeg2.o tiff.o

read-jpeg2.o: read-jpeg2.c
	gcc -std=c99 -c read-jpeg2.c -ljpeg

tiff.o: tiff.c
	gcc -std=c99 -ltiff tiff.c


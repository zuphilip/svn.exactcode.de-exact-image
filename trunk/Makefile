
all: optimize2bw empty-page

MYARCH:=$(shell uname -m)

ifeq ($(MYARCH),ppc)
OPT=-mcpu=750
else
ifeq ($(MYARCH),x86_64)
OPT=
else
OPT:=-march=$(MYARCH)
endif
endif

CFLAGS := -Wall $(OPT) -O2 -s

optimize2bw: optimize2bw.c tiff.o jpeg.o ArgumentList.o
	g++ $(CFLAGS) -I utility/include -o optimize2bw optimize2bw.c \
	    ArgumentList.o tiff.o jpeg.o -ljpeg -ltiff

empty-page: empty-page.c tiff.o ArgumentList.o
	g++ $(CFLAGS) -I utility/include -o empty-page empty-page.c \
	    ArgumentList.o tiff.o -ltiff

jpeg.o: jpeg.c
	g++ $(CFLAGS) -c jpeg.c

tiff.o: tiff.c
	g++ $(CFLAGS) -c tiff.c


ArgumentList.o: utility/src/ArgumentList.cc utility/include/ArgumentList.hh
	g++ $(CFLAGS) -I utility/include -c utility/src/ArgumentList.cc

bw-optimize: bw-optimize.c
	g++ `Wand-config --cflags --cppflags` bw-optimize.c \
	    `Wand-config --ldflags --libs` -o bw-optimize

# check:
#	display -size 1275x2096 -depth 8 gray:test.raw

clean:
	rm -rf optimize2bw *.o *tar.*

rel:= ec-$(shell date '+%Y%m%d-%H%M')
dir:= $(shell pwd)

release: clean
	rm -rf ec-[0-9]* /tmp/$(rel) ; mkdir /tmp/$(rel)
	cp -arv * /tmp/$(rel)
	cd /tmp/$(rel) ; \
	find utility/ ! -name "ArgumentList.*" -a ! -name Compiler.hh | xargs rm -f ; \
	cd .. ; \
	tar cvfz $(dir)/$(rel).tar.gz --exclude .svn --exclude '*~' $(rel)
	rm -rf /tmp/$(rel)

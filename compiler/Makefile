
ifeq ($(shell uname -s), Linux)
PRGNAME = trpc
OPT1 =
OPT2 =
else
PRGNAME = trpc.exe
OPT1 = -DMINGW
OPT2 = -lregex
endif

all:	$(PRGNAME)

$(PRGNAME):	trpc.trp lex.tin expr.tin test.tin \
		expr-gtk.tin expr-iup.tin expr-thread.tin expr-license.tin \
		expr-str.tin expr-gcrypt.tin expr-chess.tin expr-sqlite3.tin \
		expr-pix.tin expr-sift.tin expr-curl.tin expr-aud.tin \
		expr-vid.tin expr-avi.tin expr-id3tag.tin expr-magic.tin \
		expr-exif.tin expr-quirc.tin expr-wn.tin expr-avcodec.tin \
		expr-mgl.tin expr-rsvg.tin expr-qoi.tin expr-webp.tin \
		expr-openjp2.tin expr-cairo.tin expr-lept.tin expr-suf.tin \
		expr-cgraph.tin expr-microhttpd.tin \
		test-gtk.tin test-iup.tin test-thread.tin test-gcrypt.tin \
		test-minizip.tin test-chess.tin test-sqlite3.tin \
		test-magic.tin test-pix.tin test-curl.tin test-aud.tin \
		test-vid.tin test-avi.tin test-avcodec.tin test-mgl.tin \
		test-lept.tin test-suf.tin test-sdl.tin test-microhttpd.tin \
		test-qoi.tin test-webp.tin test-openjp2.tin test-cairo.tin
	trpc trpc.trp

clean:
	rm -f *~ trpc.log

cleanall:	clean
	rm -f $(PRGNAME) trpc-compile.sh trpc.c

ifeq ($(shell uname -s), Linux)
install:	all ../.installbin
	cp -f $(PRGNAME) `cat ../.installbin`
else
install:	all ../.installbin
	cp -f $(PRGNAME) `cat ../.installbin`
endif

bootstrap:	trpc.c
	gcc -O3 -pipe -Wno-pointer-sign $(OPT1) -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -fPIC -I/usr/local/include -I/opt/local/include -o $(PRGNAME) trpc.c -L/usr/local/lib -L/opt/local/lib -ltrprsvg `pkg-config librsvg-2.0 --libs` -ltrppix `pkg-config libpng --libs` -ljpeg -lgif -ltrp -lgmp `pkg-config bdw-gc --libs` -lz -lm $(OPT2)
	strip -s $(PRGNAME)

../.installbin:
	( cd .. && make dumpflags )


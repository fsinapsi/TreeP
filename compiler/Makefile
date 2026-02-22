
ifeq ($(shell uname -s), Linux)
PRGNAME = trpc
else
PRGNAME = trpc.exe
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
		expr-sdl.tin expr-cgraph.tin expr-microhttpd.tin \
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

ifeq ($(shell uname -s), Linux)
bootstrap:	trpc.c
	gcc -O3 -pipe -Wno-incompatible-pointer-types -Wno-pointer-sign -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -fPIC -I/usr/local/include -I/opt/local/include -o trpc trpc.c -L/usr/local/lib -L/opt/local/lib -ltrpwebp `pkg-config libwebp --libs` -ltrpqoi -ltrpopenjp2 `pkg-config libopenjp2 --libs` `pkg-config lcms2 --libs` -ltrprsvg `pkg-config librsvg-2.0 --libs` -ltrpheif `pkg-config libheif --libs` -ltrppix `pkg-config libpng --libs` -ljpeg -lgif -ltrp -lgmp `pkg-config bdw-gc --libs` -lz -lm
	strip -s $(PRGNAME)
else
bootstrap:	trpc.c
	gcc -O3 -pipe -Wno-incompatible-pointer-types -Wno-pointer-sign -mconsole -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DMINGW -D__WORDSIZE=64 -mms-bitfields -I/usr/local/include -o trpc.exe trpc.c -L/usr/local/lib -L/opt/local/lib -ltrpwebp `pkg-config libwebp --libs` -ltrpqoi -ltrpopenjp2 `pkg-config libopenjp2 --libs` `pkg-config lcms2 --libs` -ltrprsvg `pkg-config librsvg-2.0 --libs` -ltrpheif `pkg-config libheif --libs` -ltrppix `pkg-config libpng --libs` -ljpeg -lgif -ltrp -lgmp `pkg-config bdw-gc --libs` -lz -lm -lregex
	strip -s $(PRGNAME)
endif

../.installbin:
	( cd .. && make dumpflags )


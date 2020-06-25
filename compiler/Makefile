
UNAME := $(shell uname -s)
ifeq ($(UNAME), MINGW32_NT-5.1)
EXT=.exe
else
EXT=
endif

CC= `cat ../.mycc`
LDFLAGS=  -L/usr/local/lib -L/opt/local/lib -L../libs
LDFLAGS+= -ltrppix `pkg-config libpng --libs` -ljpeg -lgif
LDFLAGS+= -ltrp -lgmp -lgc
ifeq ($(UNAME), MINGW32_NT-5.1)
LDFLAGS+= -lpthreadGC2s -lws2_32
else
LDFLAGS+= -lpthread
endif
LDFLAGS+= -lz -lm

all:		trpc$(EXT)

trpc$(EXT):	trpc.trp ../../common/common.tin lex.tin expr.tin test.tin \
		expr-gtk.tin expr-iup.tin expr-thread.tin expr-license.tin \
		expr-gcrypt.tin expr-chess.tin expr-sqlite3.tin \
		expr-pix.tin expr-curl.tin expr-aud.tin expr-vid.tin expr-avi.tin \
		expr-id3tag.tin expr-magic.tin expr-exif.tin expr-quirc.tin expr-wn.tin \
		expr-avcodec.tin expr-cv.tin expr-vlfeat.tin expr-mgl.tin \
		expr-lept.tin expr-suf.tin expr-cgraph.tin \
		test-gtk.tin test-iup.tin test-thread.tin test-gcrypt.tin \
		test-chess.tin test-sqlite3.tin \
		test-pix.tin test-curl.tin test-aud.tin test-vid.tin test-avi.tin \
		test-avcodec.tin test-cv.tin test-vlfeat.tin test-mgl.tin \
		test-lept.tin test-suf.tin test-sdl.tin
	trpc trpc.trp

clean:
	rm -f *~ trpc-compile

cleanall:	clean
	rm -f trpc$(EXT) trpc.c

install:	all installcopy

installcopy:	../.installbin
	cp -f trpc$(EXT) `cat ../.installbin`

bootstrap: ../.mycflags
	$(CC) `cat ../.mycflags` -o trpc$(EXT) trpc.c $(LDFLAGS)

../.mycflags ../.installbin:
	( cd .. && make dumpflags )


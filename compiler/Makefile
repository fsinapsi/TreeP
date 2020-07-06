
all:	  	trpc

trpc:		trpc.trp lex.tin expr.tin test.tin \
		expr-gtk.tin expr-iup.tin expr-thread.tin expr-license.tin \
		expr-gcrypt.tin expr-chess.tin expr-sqlite3.tin \
		expr-pix.tin expr-curl.tin expr-aud.tin expr-vid.tin expr-avi.tin \
		expr-id3tag.tin expr-magic.tin expr-exif.tin expr-quirc.tin expr-wn.tin \
		expr-avcodec.tin expr-cv.tin expr-vlfeat.tin expr-mgl.tin \
		expr-lept.tin expr-suf.tin expr-cgraph.tin \
		test-gtk.tin test-iup.tin test-thread.tin test-gcrypt.tin \
		test-chess.tin test-sqlite3.tin test-magic.tin \
		test-pix.tin test-curl.tin test-aud.tin test-vid.tin test-avi.tin \
		test-avcodec.tin test-cv.tin test-vlfeat.tin test-mgl.tin \
		test-lept.tin test-suf.tin test-sdl.tin
	trpc trpc.trp

clean:
	rm -f *~ trpc.exe

cleanall:	clean
	rm -f trpc trpc-compile trpc.c

install:	all ../.installbin
	cp -f trpc `cat ../.installbin`

bootstrap:	trpc-compile trpc.c
	sh trpc-compile

../.installbin:
	( cd .. && make dumpflags )


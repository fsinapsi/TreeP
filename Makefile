
LOCAL_LINUX = /usr/local
LOCAL_WIN_32 = /home/frank/wd/programming/mingw-w64/32
LOCAL_WIN_64 = /home/frank/wd/programming/mingw-w64/64

ifeq ($(TARGET), i686-w64-mingw32)
CC = i686-w64-mingw32-gcc
AR = i686-w64-mingw32-ar
PREFIX = $(LOCAL_WIN_32)
else
ifeq ($(TARGET), x86_64-w64-mingw32)
CC = x86_64-w64-mingw32-gcc
AR = x86_64-w64-mingw32-ar
PREFIX = $(LOCAL_WIN_64)
else
TARGET = $(shell uname -s)
CC = gcc
AR = ar
PREFIX = $(LOCAL_LINUX)
endif
endif

CFLAGS ?= -O3 -pipe # -g
CFLAGS += -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -Wno-parentheses -Wno-unused-result -Wno-pointer-sign
ifeq ($(TARGET), Linux)
CFLAGS += -fPIC
CFLAGS += -I/usr/local/include -I/opt/local/include
endif
ifeq ($(TARGET), MSYS_NT-6.1-7601)
CFLAGS += -mms-bitfields
CFLAGS += -DMINGW -D__WORDSIZE=32
CFLAGS += -I/usr/local/include -I/mingw32/include
endif
ifeq ($(TARGET), i686-w64-mingw32)
CFLAGS += -mms-bitfields
CFLAGS += -DMINGW -D__WORDSIZE=32
CFLAGS += -I/usr/i686-w64-mingw32/include -I$(PREFIX)/include
endif
ifeq ($(TARGET), x86_64-w64-mingw32)
CFLAGS += -mms-bitfields
CFLAGS += -DMINGW -D__WORDSIZE=64
CFLAGS += -I/usr/x86_64-w64-mingw32/include -I$(PREFIX)/include
endif

LDFLAGS =  -L/usr/local/lib -L/opt/local/lib
LDFLAGS += -lgmp -lgc -lpthread -lz -lm

CCVER = $(shell $(CC) -v 2>&1 | grep version)

all:		rts

ifeq ($(TARGET), Linux)
install:	rts
	cp -f libs/libtrp* $(PREFIX)/lib
	ldconfig
endif

ifeq ($(TARGET), MSYS_NT-6.1-7601)
install:	rts
	cp -f libs/libtrp* $(PREFIX)/lib
endif

win32:		cleanall
	make TARGET=i686-w64-mingw32

win32-install:	win32
	cp -f libs/libtrp* $(LOCAL_WIN_32)/lib

win64:		cleanall
	make TARGET=x86_64-w64-mingw32

win64-install:	win64
	cp -f libs/libtrp* $(LOCAL_WIN_64)/lib

bootstrap:	install
	( cd compiler && make bootstrap )
	( cd compiler && make install )

dumpflags:
	echo -n $(CFLAGS) > .cflags
	echo -n $(LDFLAGS) > .ldflags
	echo -n $(PREFIX)/bin > .installbin
	echo -n "static char *_trp_cc_ver=\"$(CCVER)\";" > .ccver

rts:		dumpflags
	mkdir -p libs
	( cd trp && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trppix && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpwebp && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpqoi && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpopenjp2 && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpjbig2 && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trprsvg && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
ifeq ($(TARGET), Linux)
	( cd trpsail && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
endif
	( cd trpsift && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpthread && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trplicense && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpiup && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpsqlite3 && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trplept && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpgcrypt && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpsuf && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpaud && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpvid && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpavi && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpchess && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpcurl && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpsdl && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpquirc && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpexif && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpavcodec && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpid3tag && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpmagic && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpgtk && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpwn && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpcgraph && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpcairo && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpmicrohttpd && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
	( cd trpcv && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )
#	( cd trpmgl && make TARGET=$(TARGET) CC=$(CC) AR=$(AR) )

clean:
	rm -f *~ .cflags .ldflags .installbin .ccver
	rm -rf libs
	( cd compiler && make clean )
	( cd examples && make clean )
	( cd trp && make clean )
	( cd trppix && make clean )
	( cd trpwebp && make clean )
	( cd trpqoi && make clean )
	( cd trpopenjp2 && make clean )
	( cd trpjbig2 && make clean )
	( cd trprsvg && make clean )
	( cd trpsail && make clean )
	( cd trpsift && make clean )
	( cd trpthread && make clean )
	( cd trplicense && make clean )
	( cd trpiup && make clean )
	( cd trpsqlite3 && make clean )
	( cd trplept && make clean )
	( cd trpgcrypt && make clean )
	( cd trpsuf && make clean )
	( cd trpaud && make clean )
	( cd trpvid && make clean )
	( cd trpavi && make clean )
	( cd trpchess && make clean )
	( cd trpcurl && make clean )
	( cd trpsdl && make clean )
	( cd trpquirc && make clean )
	( cd trpexif && make clean )
	( cd trpavcodec && make clean )
	( cd trpid3tag && make clean )
	( cd trpmagic && make clean )
	( cd trpgtk && make clean )
	( cd trpwn && make clean )
	( cd trpcgraph && make clean )
	( cd trpcairo && make clean )
	( cd trpmicrohttpd && make clean )
	( cd trpcv && make clean )
	( cd trpmgl && make clean )

cleanall:	clean
	( cd compiler && make cleanall )


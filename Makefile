
UNAME := $(shell uname -s)

ifeq ($(UNAME), MINGW32_NT-5.1)
PREFIX= /local
else
PREFIX= /usr/local
endif

CC= gcc

CFLAGS?=  -O3 -pipe # -g
CFLAGS+= -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS+= -Wno-parentheses
ifeq ($(UNAME), Linux)
CFLAGS+= -fPIC -Wno-unused-result
endif
ifeq ($(UNAME), MINGW32_NT-5.1)
CFLAGS+= -mms-bitfields
else
CFLAGS+= -Wno-pointer-sign
endif
CFLAGS+= -isystem /usr/local/include -I/opt/local/include

LDFLAGS=  -L/usr/local/lib -L/opt/local/lib
LDFLAGS+= -lgmp -lgc -lpthread -lz -lm

all:		rts

install:	rts
	cp -f libs/libtrp* $(PREFIX)/lib

bootstrap:	install
	( cd compiler && make bootstrap )
	( cd compiler && make installcopy )

dumpflags:
	echo $(CC) > .mycc
	echo $(CFLAGS) > .mycflags
	echo $(LDFLAGS) > .myldflags
	echo $(PREFIX)/bin > .installbin

rts:		dumpflags
	mkdir -p libs
	( cd trp && make )
	( cd trpthread && make )
	( cd trplicense && make )
	( cd trpgcrypt && make )
	( cd trpsuf && make )
	( cd trpaud && make )
	( cd trpvid && make )
	( cd trpavi && make )
	( cd trpid3tag && make )
	( cd trpmagic && make )
	( cd trpexif && make )
	( cd trpchess && make )
	( cd trpcurl && make )
	( cd trpsqlite3 && make )
	( cd trppix && make )
	( cd trpgtk && make )
	( cd trpiup && make )
ifneq ($(UNAME), Darwin)
	( cd trpavcodec && make )
endif
ifeq ($(UNAME), Linux)
	( cd trpcgraph && make )
	( cd trpsdl && make )
	( cd trpquirc && make )
	( cd trplept && make )
endif

clean:
	rm -f *~ .mycc .mycflags .myldflags .installbin libs/*
	( cd compiler && make clean )
	( cd examples && make clean )
	( cd trp && make clean )
	( cd trpthread && make clean )
	( cd trplicense && make clean )
	( cd trpgcrypt && make clean )
	( cd trpsuf && make clean )
	( cd trpaud && make clean )
	( cd trpvid && make clean )
	( cd trpavi && make clean )
	( cd trpid3tag && make clean )
	( cd trpmagic && make clean )
	( cd trpexif && make clean )
	( cd trpcgraph && make clean )
	( cd trpsdl && make clean )
	( cd trpquirc && make clean )
	( cd trpchess && make clean )
	( cd trpcurl && make clean )
	( cd trpsqlite3 && make clean )
	( cd trppix && make clean )
	( cd trpgtk && make clean )
	( cd trpiup && make clean )
	( cd trpavcodec && make clean )
	( cd trplept && make clean )

cleanall:	clean
	( cd compiler && make cleanall )


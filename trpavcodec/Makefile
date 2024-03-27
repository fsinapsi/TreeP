
myname=	avcodec
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o

CFLAGS= `cat ../.cflags`
ifeq ($(TARGET), i686-w64-mingw32)
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags libavformat`
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags libavcodec`
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags libavutil`
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags libswscale`
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags libavfilter`
else
ifeq ($(TARGET), x86_64-w64-mingw32)
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags libavformat`
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags libavcodec`
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags libavutil`
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags libswscale`
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags libavfilter`
else
CFLAGS+= `pkg-config --cflags libavformat`
CFLAGS+= `pkg-config --cflags libavcodec`
CFLAGS+= `pkg-config --cflags libavutil`
CFLAGS+= `pkg-config --cflags libswscale`
CFLAGS+= `pkg-config --cflags libavfilter`
endif
endif
LDFLAGS= `cat ../.ldflags`

all:			$(mylibs)

clean:
	rm -f *~ $(mylibs) $(myobjs)

$(mylibs):		$(myobjs)
	rm -f $(mylibs)
	$(AR) rcs ../libs/libtrp$(myname).a $(myobjs)
ifeq ($(TARGET), Linux)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libtrp$(myname).so -o ../libs/libtrp$(myname).so $(myobjs)
endif

$(myobjs):		../trp/trp.h trp$(myname).h

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@


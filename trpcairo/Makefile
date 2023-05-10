
myname=	cairo
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o

CFLAGS= `cat ../.cflags`
ifeq ($(TARGET), Linux)
CFLAGS+= `pkg-config --cflags cairo`
CFLAGS+= `pkg-config --cflags freetype2`
CFLAGS+= `pkg-config --cflags librsvg-2.0`
endif
ifeq ($(TARGET), MSYS_NT-6.1-7601)
CFLAGS+= `pkg-config --cflags cairo`
CFLAGS+= `pkg-config --cflags freetype2`
CFLAGS+= `pkg-config --cflags librsvg-2.0`
endif
ifeq ($(TARGET), i686-w64-mingw32)
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags cairo`
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags freetype2`
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags /home/frank/wd/programming/mingw-w64/32/lib/pkgconfig/librsvg-2.0.pc`
endif
ifeq ($(TARGET), x86_64-w64-mingw32)
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags cairo`
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags freetype2`
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags /home/frank/wd/programming/mingw-w64/64/lib/pkgconfig/librsvg-2.0.pc`
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


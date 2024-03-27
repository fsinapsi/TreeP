
myname=	openjp2
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o color.o

CFLAGS= `cat ../.cflags`
ifeq ($(TARGET), i686-w64-mingw32)
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags libopenjp2` `pkg-config --cflags lcms2`
else
ifeq ($(TARGET), x86_64-w64-mingw32)
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags libopenjp2` `pkg-config --cflags lcms2`
else
CFLAGS+= `pkg-config --cflags libopenjp2` `pkg-config --cflags lcms2`
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


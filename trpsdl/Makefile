
UNAME := $(shell uname -s)

myname=	sdl
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o
CC= `cat ../.mycc`
CFLAGS= -c `cat ../.mycflags` `pkg-config --cflags sdl2`
LDFLAGS= `cat ../.myldflags`

all:			$(mylibs)

clean:
	rm -f *~ $(mylibs) $(myobjs)

$(mylibs):		$(myobjs)
	rm -f $(mylibs)
	ar rcs ../libs/libtrp$(myname).a $(myobjs)
ifeq ($(UNAME), Linux)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libtrp$(myname).so -o ../libs/libtrp$(myname).so $(myobjs)
endif

$(myobjs):		../trp/trp.h trp$(myname).h

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@



myname=	wn
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o binsrch.o morph.o search.o wnglobal.o wnhelp.o wnrtl.o wnutil.o

CFLAGS= `cat ../.cflags`
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

trpwn.o:		../trp/trp.h trp$(myname).h

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@


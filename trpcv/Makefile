
myname=	cv
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o sift/imgfeatures.o sift/utils.o sift/sift.o sift/kdtree.o sift/minpq.o sift/xform.o

CFLAGS= `cat ../.cflags` `pkg-config --cflags opencv`
LDFLAGS= `cat ../.ldflags`

all:			$(mylibs)

clean:
	rm -f *~ sift/*~ $(mylibs) $(myobjs)

$(mylibs):		$(myobjs)
	rm -f $(mylibs)
	$(AR) rcs ../libs/libtrp$(myname).a $(myobjs)
ifeq ($(TARGET), Linux)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libtrp$(myname).so -o ../libs/libtrp$(myname).so $(myobjs)
endif

$(myobjs):		../trp/trp.h trp$(myname).h

$%.o: %.c ../trppix/trppix_internal.h
	$(CC) $< $(CFLAGS) -c -o $@


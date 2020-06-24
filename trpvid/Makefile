
UNAME := $(shell uname -s)

myname=	 vid
mylibs=	 ../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	 trp$(myname).o trpvid_api.o
myobjs+= trpvid_msmpeg4.o trpvid_mpeg4asp.o trpvid_mpeg4avc.o trpvid_mp4.o
myobjs+= trpvid_qscale_correction.o trpvid_search.o vlc.o
CC= `cat ../.mycc`
CFLAGS= -c `cat ../.mycflags`
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

$(myobjs):		../trp/trp.h trp$(myname).h trpvid_internal.h

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@


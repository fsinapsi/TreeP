
UNAME := $(shell uname -s)

myname=	pix
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o epeg_main.o
myobjs+= trppix_conv.o trppix_png.o trppix_jpg.o trppix_pnm.o
myobjs+= trppix_gif.o trppix_yuv4mpeg2.o
myobjs+= trppix_draw.o trppix_misc.o trppix_text.o
myobjs+= trppix_ssim.o trppix_scd.o trppix_stat.o
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

$(myobjs):		../trp/trp.h trp$(myname).h trppix_internal.h

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@


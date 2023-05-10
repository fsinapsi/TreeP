
myname=	pix
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o epeg_main.o
myobjs+= trppix_features.o trppix_load.o trppix_edit.o trppix_conv.o
myobjs+= trppix_scale.o trppix_rotate.o
myobjs+= trppix_fmt_png.o trppix_fmt_jpg.o trppix_fmt_pnm.o trppix_fmt_gif.o
myobjs+= trppix_fmt_tga.o trppix_fmt_xpm.o trppix_fmt_ptg.o
myobjs+= trppix_fmt_yuv4mpeg2.o
myobjs+= trppix_color_transfer.o trppix_draw.o trppix_misc.o trppix_text.o
myobjs+= trppix_ssim.o trppix_scd.o trppix_stat.o trppix_avn_graph.o

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

$(myobjs):		../trp/trp.h trp$(myname).h trppix_internal.h

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@


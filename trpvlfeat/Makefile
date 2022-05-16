
myname=	vlfeat
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o vl/sift.o vl/kdtree.o vl/generic.o vl/random.o vl/host.o vl/imopv.o vl/mathop.o
ifneq ($(TARGET), i686-w64-mingw32)
ifneq ($(TARGET), MSYS_NT-6.1-7601)
myobjs+= vl/imopv_sse2.o vl/mathop_sse2.o
endif
endif

CFLAGS= `cat ../.cflags` -DVL_DISABLE_AVX -DVL_DISABLE_THREADS -DVL_DISABLE_OPENMP
ifeq ($(TARGET), i686-w64-mingw32)
CFLAGS += -DVL_DISABLE_SSE2
endif
ifeq ($(TARGET), x86_64-w64-mingw32)
CFLAGS += -D__LP64
endif
ifeq ($(TARGET), MSYS_NT-6.1-7601)
CFLAGS += -DVL_DISABLE_SSE2
endif
LDFLAGS= `cat ../.ldflags`

all:			$(mylibs)

clean:
	rm -f *~ $(mylibs) $(myobjs) vl/*~

$(mylibs):		$(myobjs)
	rm -f $(mylibs)
	$(AR) rcs ../libs/libtrp$(myname).a $(myobjs)
ifeq ($(TARGET), Linux)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libtrp$(myname).so -o ../libs/libtrp$(myname).so $(myobjs)
endif

$(myobjs):		../trp/trp.h trp$(myname).h

$%.o: %.c ../trppix/trppix_internal.h
	$(CC) $< $(CFLAGS) -c -o $@


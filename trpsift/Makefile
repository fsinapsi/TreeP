
myname=	sift
mylibs=	../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	trp$(myname).o libsiftfast.o

CFLAGS= `cat ../.cflags`
#CFLAGS+= -DTRP_PRINT_RUSAGE_DIFF
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

$(myobjs):		../trp/trp.h trp$(myname).h libsiftfast.h

ifeq ($(TARGET), i686-w64-mingw32)
libsiftfast.o: libsiftfast.cpp
	i686-w64-mingw32-g++ -D_FORTIFY_SOURCE=2 -D_GLIBCXX_ASSERTIONS -O3 -pipe -fno-plt -fexceptions --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fcf-protection -Wall -Wno-narrowing -fPIC -msse2 -mfpmath=sse -fopenmp -c libsiftfast.cpp
else
ifeq ($(TARGET), x86_64-w64-mingw32)
libsiftfast.o: libsiftfast.cpp
	x86_64-w64-mingw32-g++ -D__x86_64__ -D_FORTIFY_SOURCE=2 -D_GLIBCXX_ASSERTIONS -O3 -pipe -fno-plt -fexceptions --param=ssp-buffer-size=4 -Wformat -Werror=format-security -fcf-protection -Wall -Wno-narrowing -fPIC -msse2 -mfpmath=sse -fopenmp -c libsiftfast.cpp
else
libsiftfast.o: libsiftfast.cpp
	g++ -D__x86_64__ -O3 -pipe -Wno-narrowing -fPIC -msse2 -mfpmath=sse -fopenmp -c libsiftfast.cpp
endif
endif

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@


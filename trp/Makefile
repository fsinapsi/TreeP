
UNAME := $(shell uname -s)

mylibs=	../libs/libtrp.a ../libs/libtrp.so
myobjs=	trp.o \
	aks.o cordbscs.o cordxtra.o \
	trp_arg.o trp_cast.o trp_functions.o trp_print.o trp_relations.o \
	trp_support.o trp_misc.o trp_t_special.o trp_t_raw.o trp_t_char.o \
	trp_t_date.o trp_t_file.o trp_t_math.o trp_t_list.o trp_t_array.o \
	trp_t_queue.o trp_t_stack.o trp_t_cord.o trp_t_tree.o trp_t_assoc.o \
	trp_t_funptr.o trp_t_netptr.o trp_t_regex.o trp_t_fibo.o
CC= `cat ../.mycc`
CFLAGS= -c `cat ../.mycflags`
LDFLAGS= `cat ../.myldflags`

all:			$(mylibs)

clean:
	rm -f *~ private/*~ $(mylibs) $(myobjs)

$(mylibs):		$(myobjs)
	rm -f $(mylibs)
	ar rcs ../libs/libtrp.a $(myobjs)
ifeq ($(UNAME), Linux)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libtrp.so -o ../libs/libtrp.so $(myobjs)
endif

$(myobjs):		trp.h

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@


/*
    TreeP Run Time Support
    Copyright (C) 2008-2025 Frank Sinapsi

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __trp__h
#define __trp__h

/*
 macro eventualmente da ridefinire -- inizio
 */

#ifdef __BIG_ENDIAN__
#  undef TRP_LITTLE_ENDIAN
#  define TRP_BIG_ENDIAN
#else /* __BIG_ENDIAN__ */
#ifdef _LIBC
# include <endian.h>
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define TRP_LITTLE_ENDIAN
#  undef TRP_BIG_ENDIAN
# else
#  undef TRP_LITTLE_ENDIAN
#  define TRP_BIG_ENDIAN
# endif
#else /* _LIBC */
# if 1
#  define TRP_LITTLE_ENDIAN
#  undef TRP_BIG_ENDIAN
# else
#  undef TRP_LITTLE_ENDIAN
#  define TRP_BIG_ENDIAN
# endif
#endif /* _LIBC */
#endif /* __BIG_ENDIAN__ */

#define uns8b unsigned char
#define uns16b unsigned short int
#define uns32b unsigned int
#define uns64b unsigned long long int

#define sig8b signed char
#define sig16b signed short int
#define sig32b signed int
#define sig64b signed long long int

#define flt32b float
#define flt64b double

#define TRP_FORCE_FREE

/*
 macro eventualmente da ridefinire -- fine
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <regex.h>
#include <gmp.h>
#ifndef MINGW
#include <sys/wait.h>
#include <sys/socket.h>
#include <poll.h>
#else
#define GC_WIN32_PTHREADS
#endif
#define GC_THREADS
#define CORD_BUILD
#define I_HIDE_POINTERS
#include <gc/gc.h>
#include <gc/cord.h>
#include <gc/cord_pos.h>
#include <gc/ec.h>

enum {
    TRP_SPECIAL = 0,
    TRP_RAW,
    TRP_CHAR,
    TRP_DATE,
    TRP_FILE,
    TRP_SIG64,
    TRP_MPI,
    TRP_RATIO,
    TRP_COMPLEX,
    TRP_CONS,
    TRP_ARRAY,
    TRP_QUEUE,
    TRP_STACK,
    TRP_CORD,
    TRP_TREE,
    TRP_DGRAPH,
    TRP_FUNPTR,
    TRP_NETPTR,
    TRP_THREAD,
    TRP_CURL,
    TRP_PIX,
    TRP_ASSOC,
    TRP_SET,
    TRP_SQLITE3,
    TRP_AUD,
    TRP_VID,
    TRP_GTK,
    TRP_AVI,
    TRP_AVCODEC,
    TRP_OPENCV,
    TRP_SIFT,
    TRP_SUF,
    TRP_MGL,
    TRP_IUP,
    TRP_REGEX,
    TRP_FIBO,
    TRP_CHESS,
    TRP_CAIRO,
    TRP_MHD,
    TRP_DBF,
    TRP_MAX_T /* lasciarlo sempre per ultimo */
};

#define UNDEF        trp_undef()
#define NIL          trp_nil()
#define TRP_TRUE     trp_true()
#define TRP_FALSE    trp_false()
#define NL           trp_nl()
#define EMPTYCORD    trp_cord_empty()
#define ZERO         trp_zero()
#define UNO          trp_uno()
#define DIECI        trp_dieci()
#define TRP_MAXINT   trp_maxint()
#define TRP_MININT   trp_minint()
#define TRP_EQUAL    trp_funptr_equal_obj()
#define TRP_LESS     trp_funptr_less_obj()
#define TRP_STDIN    trp_stdin()
#define TRP_STDOUT   trp_stdout()
#define TRP_STDERR   trp_stderr()

#define TRP_BOOLP(x) (((x)==TRP_TRUE)||((x)==TRP_FALSE))

#define TRP_EQP(x,y) (((x)==(y))?TRP_TRUE:TRP_FALSE)

#ifdef TRP_FORCE_FREE
#define trp_gc_free(p) GC_free((void *)(p))
#else
#define trp_gc_free(p)
#endif

#define TRP_MAX(a,b) (((a)>=(b))?(a):(b))
#define TRP_MIN(a,b) (((a)<=(b))?(a):(b))
#define TRP_ABS(a) (((a)>=0.0)?(a):-(a))
#define TRP_ABSDIFF(a,b) (((a)>=(b))?((a)-(b)):((b)-(a)))

typedef struct {
    uns8b flags;
    FILE *fp;
    uns8b *buf;
    uns32b cnt;
    CORD_ec x;
} trp_print_t;

typedef struct {
    uns8b tipo;
} trp_obj_t;

typedef void (*voidfun_t)( ... );
typedef int (*intfun_t)( ... );
typedef flt64b (*doublefun_t)( ... );
typedef uns8b (*uns8bfun_t)( ... );
typedef uns32b (*uns32bfun_t)( ... );
typedef trp_obj_t * (*objfun_t)( ... );

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
} trp_special_t;

typedef struct {
    uns8b tipo;
    uns8b mode;
    uns8b unc_tipo;
    uns8b compression_level;
    uns32b len;
    uns32b unc_len;
    uns8b *data;
} trp_raw_t;

typedef struct {
    uns8b tipo;
    uns8b c;
} trp_char_t;

typedef struct {
    uns8b tipo;
    uns16b anno;
    uns8b mese;
    uns8b giorno;
    uns8b ore;
    uns8b minuti;
    uns8b secondi;
    trp_obj_t *resto;
    sig32b tz;
} trp_date_t;

typedef struct {
    uns8b tipo;
    uns8b flags;
    pthread_mutex_t mutex;
    FILE *fp;
    uns32b line;
    uns8b last;
} trp_file_t;

typedef struct {
    uns8b tipo;
    sig64b val;
} trp_sig64_t;

typedef struct {
    uns8b tipo;
    mpz_t val;
} trp_mpi_t;

typedef struct {
    uns8b tipo;
    mpq_t val;
} trp_ratio_t;

typedef struct {
    uns8b tipo;
    trp_obj_t *re;
    trp_obj_t *im;
} trp_complex_t;

typedef struct {
    uns8b tipo;
    trp_obj_t *car;
    trp_obj_t *cdr;
} trp_cons_t;

typedef struct {
    uns8b tipo;
    uns32b incr;
    uns32b len;
    trp_obj_t **data;
} trp_array_t;

typedef struct {
    uns8b tipo;
    uns32b len;
    void *first;
    void *last;
} trp_queue_t;

typedef struct {
    uns8b tipo;
    uns32b len;
    void *data;
} trp_stack_t;

typedef struct {
    uns8b tipo;
    uns32b len;
    CORD c;
} trp_cord_t;

typedef struct {
    uns8b tipo;
    trp_obj_t *val;
    trp_obj_t *parent;
    trp_array_t *children;
} trp_tree_t;

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    uns32b len;
    uns32b link_cnt;
    void *root_nodes;
    trp_obj_t *names;
} trp_dgraph_t;

typedef struct {
    uns8b tipo;
    uns32b len;
    void *root;
} trp_assoc_t;

typedef struct {
    uns8b tipo;
    uns32b len;
    void *root;
} trp_set_t;

typedef struct {
    uns8b tipo;
    objfun_t f;
    uns8b nargs;
    trp_obj_t *name;
} trp_funptr_t;

typedef struct {
    uns8b tipo;
    uns8bfun_t f;
    uns8b nargs;
    trp_obj_t *name;
} trp_netptr_t;

typedef struct {
    uns8b tipo;
    regex_t *preg;
} trp_regex_t;

typedef struct trp_fibo_node_t {
    uns8b tipo;
    uns8b sottotipo;
    void *fibo;
    trp_obj_t *key;
    trp_obj_t *obj;
    struct trp_fibo_node_t *p;
    struct trp_fibo_node_t *child;
    struct trp_fibo_node_t *left;
    struct trp_fibo_node_t *right;
    uns32b degree; /* the number of its children */
    uns8b marked; /* indicates whether the node lost a child */
} trp_fibo_node_t;

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    uns32b len;
    trp_fibo_node_t *min; /* pointer pointing to min key in the heap */
    objfun_t cmp;
} trp_fibo_t;

void trp_init( int argc, char *argv[] );
void trp_exit( trp_obj_t *obj );
trp_obj_t *trp_heap_size();
trp_obj_t *trp_free_bytes();
trp_obj_t *trp_endianness();
char *trp_gc_strdup( const char *s );
void *trp_gc_calloc( size_t nmemb, size_t size );
void *trp_gc_malloc( size_t size );
void *trp_gc_malloc_finalize( size_t size, GC_finalization_proc fn );
void *trp_gc_malloc_atomic( size_t size );
void *trp_gc_malloc_atomic_finalize( size_t size, GC_finalization_proc fn );
void *trp_gc_realloc( void *p, size_t size );
void trp_gc_remove_finalizer( trp_obj_t *obj );
void *trp_malloc( size_t size );
void *trp_realloc( void *p, size_t size );
#ifdef TRP_FORCE_FREE
void trp_free_list( trp_obj_t *l );
#else /* TRP_FORCE_FREE */
#define trp_free_list(l)
#endif /* TRP_FORCE_FREE */
uns16b trp_swap_endian16( uns16b n );
uns32b trp_swap_endian32( uns32b n );
uns64b trp_swap_endian64( uns64b n );
#ifdef TRP_LITTLE_ENDIAN
#define norm16(n) ((uns16b)(n))
#define norm32(n) ((uns32b)(n))
#define norm64(n) ((uns64b)(n))
#else /* TRP_LITTLE_ENDIAN */
#define norm16(n) trp_swap_endian16((uns16b)(n))
#define norm32(n) trp_swap_endian32((uns32b)(n))
#define norm64(n) trp_swap_endian64((uns64b)(n))
#endif /* TRP_LITTLE_ENDIAN */
uns32b trp_nargs( va_list args );
uns8b trp_upcase( uns8b c );
uns8b trp_downcase( uns8b c );
void trp_skip( trp_obj_t *obj );
void trp_segfault();
trp_obj_t *trp_cc_version();

uns8b trp_cast_uns32b( trp_obj_t *obj, uns32b *val );
uns8b trp_cast_uns32b_range( trp_obj_t *obj, uns32b *val, uns32b min, uns32b max );
uns8b trp_cast_uns32b_rint( trp_obj_t *obj, uns32b *val );
uns8b trp_cast_uns32b_rint_range( trp_obj_t *obj, uns32b *val, uns32b min, uns32b max );
uns8b trp_cast_sig32b( trp_obj_t *obj, sig32b *val );
uns8b trp_cast_sig32b_range( trp_obj_t *obj, sig32b *val, sig32b min, sig32b max );
uns8b trp_cast_sig64b( trp_obj_t *obj, sig64b *val );
uns8b trp_cast_sig64b_range( trp_obj_t *obj, sig64b *val, sig64b min, sig64b max );
uns8b trp_cast_sig64b_rint( trp_obj_t *obj, sig64b *val );
uns8b trp_cast_sig64b_rint_range( trp_obj_t *obj, sig64b *val, sig64b min, sig64b max );
uns8b trp_cast_flt64b( trp_obj_t *obj, flt64b *val );
uns8b trp_cast_flt64b_range( trp_obj_t *obj, flt64b *val, flt64b min, flt64b max );
#define trp_cast_double(a,b) trp_cast_flt64b(a,b)
#define trp_cast_double_range(a,b,c,d) trp_cast_flt64b_range(a,b,c,d)

void trp_arg_init( int argc, char *argv[] );
trp_obj_t *trp_argc();
trp_obj_t *trp_argv( trp_obj_t *obj );

uns8b trp_close( trp_obj_t *obj );
uns8b trp_close_multi( trp_obj_t *obj, ... );
trp_obj_t *trp_length( trp_obj_t *obj );
trp_obj_t *trp_width( trp_obj_t *obj );
trp_obj_t *trp_height( trp_obj_t *obj );
trp_obj_t *trp_nth( trp_obj_t *n, trp_obj_t *obj );
trp_obj_t *trp_sub( trp_obj_t *start, trp_obj_t *len, trp_obj_t *obj );
trp_obj_t *trp_cat( trp_obj_t *obj, ... );
trp_obj_t *trp_in_func( trp_obj_t *obj, trp_obj_t *seq, trp_obj_t *interv );
uns8b trp_in_test( trp_obj_t *obj, trp_obj_t *seq, trp_obj_t *interv, trp_obj_t **pos, trp_obj_t *nth );
trp_obj_t *trp_reverse( trp_obj_t *obj );
trp_obj_t *trp_typeof( trp_obj_t *obj );
#define trp_typec() trp_sig64(TRP_MAX_T)
trp_obj_t *trp_typev( trp_obj_t *obj );
trp_obj_t *trp_integerp( trp_obj_t *obj );
trp_obj_t *trp_rationalp( trp_obj_t *obj );
trp_obj_t *trp_complexp( trp_obj_t *obj );
trp_obj_t *trp_listp( trp_obj_t *obj );
trp_obj_t *trp_booleanp( trp_obj_t *obj );
#define trp_stringp(p) (((p)->tipo==TRP_CORD)?TRP_TRUE:TRP_FALSE)
#define trp_rawp(p) (((p)->tipo==TRP_RAW)?TRP_TRUE:TRP_FALSE)
#define trp_charp(p) (((p)->tipo==TRP_CHAR)?TRP_TRUE:TRP_FALSE)
#define trp_datep(p) (((p)->tipo==TRP_DATE)?TRP_TRUE:TRP_FALSE)
#define trp_queuep(p) (((p)->tipo==TRP_QUEUE)?TRP_TRUE:TRP_FALSE)
#define trp_arrayp(p) (((p)->tipo==TRP_ARRAY)?TRP_TRUE:TRP_FALSE)
#define trp_assocp(p) (((p)->tipo==TRP_ASSOC)?TRP_TRUE:TRP_FALSE)
#define trp_setp(p) (((p)->tipo==TRP_SET)?TRP_TRUE:TRP_FALSE)
#define trp_treep(p) (((p)->tipo==TRP_TREE)?TRP_TRUE:TRP_FALSE)
#define trp_dgraphp(p) (((p)->tipo==TRP_DGRAPH)?TRP_TRUE:TRP_FALSE)
#define trp_fibop(p) (((p)->tipo==TRP_FIBO)?TRP_TRUE:TRP_FALSE)
#define trp_pixp(p) (((p)->tipo==TRP_PIX)?TRP_TRUE:TRP_FALSE)
#define trp_threadp(p) (((p)->tipo==TRP_THREAD)?TRP_TRUE:TRP_FALSE)
#define trp_gtkp(p) (((p)->tipo==TRP_GTK)?TRP_TRUE:TRP_FALSE)
#define trp_audp(p) (((p)->tipo==TRP_AUD)?TRP_TRUE:TRP_FALSE)
#define trp_vidp(p) (((p)->tipo==TRP_VID)?TRP_TRUE:TRP_FALSE)

uns8b trp_print( trp_obj_t *obj, ... );
uns8b trp_fprint( trp_obj_t *stream, trp_obj_t *obj, ... );
trp_obj_t *trp_sprint( trp_obj_t *obj, ... );
trp_obj_t *trp_sprint_list( trp_obj_t *l, trp_obj_t *divider );
uns8b *trp_csprint( trp_obj_t *obj );
uns8b *trp_csprint_multi( trp_obj_t *obj, va_list args );
#ifdef TRP_FORCE_FREE
void trp_csprint_free( uns8b *p );
#else
#define trp_csprint_free(p)
#endif
uns8b trp_print_dump_raw( trp_obj_t *stream, trp_obj_t *raw );
uns8b trp_print_obj( trp_print_t *p, trp_obj_t *obj );
uns8b trp_print_chars( trp_print_t *p, uns8b *s, uns32b cnt );
uns8b trp_print_char_star( trp_print_t *p, uns8b *s );
uns8b trp_print_sig64( trp_print_t *p, sig64b val );
uns8b trp_print_char( trp_print_t *p, uns8b c );

trp_obj_t *trp_equal( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_notequal( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_less( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_greater( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_less_or_equal( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_greater_or_equal( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_min( trp_obj_t *obj, ... );
trp_obj_t *trp_max( trp_obj_t *obj, ... );
trp_obj_t *trp_or( trp_obj_t *obj, ... );
trp_obj_t *trp_and( trp_obj_t *obj, ... );
trp_obj_t *trp_not( trp_obj_t *obj );

void trp_const_init( uns32b n, trp_raw_t r[], uns8b *c[], uns64b cst_totsize );
void trp_glb_init( uns32b n, trp_obj_t *glb[] );
void trp_gc();
trp_obj_t *trp_const( uns32b i );
void trp_push_env( trp_obj_t *obj, ... );
void trp_pop_env( trp_obj_t **obj, ... );
void trp_pop_env_void( uns32b n );
uns8b trp_for_init( trp_obj_t **fst, trp_obj_t **var, trp_obj_t *from, trp_obj_t *to, trp_obj_t *step, uns8b rev );
uns8b trp_for_next( trp_obj_t **fst );
void trp_for_break( trp_obj_t **fst );
trp_obj_t *trp_for_pos( trp_obj_t *fst );

#ifdef MINGW
void trp_convert_slash( uns8b *p );
wchar_t *trp_utf8_to_wc( const uns8b *p );
wchar_t *trp_utf8_to_wc_path( uns8b *cpath );
uns8b *trp_wc_to_utf8( const wchar_t *wp );
FILE *trp_fopen( const char *path, const char *mode );
int trp_open( const char *path, int oflag );
uns8b *trp_get_short_path_name( uns8b *path );
#else
#define trp_convert_slash(p)
#define trp_fopen(p,m) fopen(p,m)
#define trp_open(p,m) open(p,m)
#define trp_get_short_path_name(p) (p)
#endif
trp_obj_t *trp_uname();
trp_obj_t *trp_getrusage_self();
trp_obj_t *trp_getrusage_children();
trp_obj_t *trp_getrusage_thread();
#define trp_getrusage() trp_getrusage_self()
trp_obj_t *trp_getuid();
trp_obj_t *trp_geteuid();
trp_obj_t *trp_realpath( trp_obj_t *obj );
trp_obj_t *trp_cwd();
uns8b trp_chdir( trp_obj_t *path );
uns8b trp_mkdir( trp_obj_t *path );
uns8b trp_mkfifo( trp_obj_t *path );
uns8b trp_remove( trp_obj_t *path );
uns8b trp_rename( trp_obj_t *oldp, trp_obj_t *newp );
trp_obj_t *trp_pathexists( trp_obj_t *path );
trp_obj_t *trp_ftime( trp_obj_t *path );
trp_obj_t *trp_fsize( trp_obj_t *path );
uns8b trp_utime( trp_obj_t *path, trp_obj_t *actime, trp_obj_t *modtime );
trp_obj_t *trp_directory( trp_obj_t *obj );
trp_obj_t *trp_directory_ext( trp_obj_t *obj );
trp_obj_t *trp_getenv( trp_obj_t *obj );
uns8b trp_sleep( trp_obj_t *sec );
trp_obj_t *trp_lstat_mode( trp_obj_t *path );
trp_obj_t *trp_isreg( trp_obj_t *path );
trp_obj_t *trp_isdir( trp_obj_t *path );
trp_obj_t *trp_ischr( trp_obj_t *path );
trp_obj_t *trp_isblk( trp_obj_t *path );
trp_obj_t *trp_isfifo( trp_obj_t *path );
trp_obj_t *trp_islnk( trp_obj_t *path );
trp_obj_t *trp_issock( trp_obj_t *path );
trp_obj_t *trp_inode( trp_obj_t *path );
trp_obj_t *trp_gc_version_major();
trp_obj_t *trp_gc_version_minor();
trp_obj_t *trp_readlink( trp_obj_t *path );
uns8b trp_link( trp_obj_t *path1, trp_obj_t *path2 );
uns8b trp_symlink( trp_obj_t *path1, trp_obj_t *path2 );
void trp_sync();
trp_obj_t *trp_ipv4_address();
trp_obj_t *trp_system( trp_obj_t *obj, ... );
trp_obj_t *trp_getpid();
trp_obj_t *trp_fork();
trp_obj_t *trp_sysinfo();
trp_obj_t *trp_ratio2uns64b( trp_obj_t *obj );
void trp_print_rusage_diff( char *msg );

uns8b trp_special_print( trp_print_t *p, trp_special_t *obj );
uns32b trp_special_size( trp_special_t *obj );
void trp_special_encode( trp_special_t *obj, uns8b **buf );
trp_obj_t *trp_special_decode( uns8b **buf );
trp_obj_t *trp_special_equal( trp_special_t *o1, trp_special_t *o2 );
trp_obj_t *trp_special_less( trp_special_t *o1, trp_special_t *o2 );
trp_obj_t *trp_special_length( trp_obj_t *obj );
trp_obj_t *trp_special_sub( uns32b start, uns32b len, trp_obj_t *obj );
void trp_special_init();
trp_obj_t *trp_undef();
trp_obj_t *trp_nil();
trp_obj_t *trp_true();
trp_obj_t *trp_false();

uns8b trp_raw_print( trp_print_t *p, trp_raw_t *obj );
uns32b trp_raw_size( trp_raw_t *obj );
void trp_raw_encode( trp_raw_t *obj, uns8b **buf );
trp_obj_t *trp_raw_decode( uns8b **buf );
trp_obj_t *trp_raw_equal( trp_raw_t *o1, trp_raw_t *o2 );
trp_obj_t *trp_raw_length( trp_raw_t *obj );
trp_obj_t *trp_raw_nth( uns32b n, trp_raw_t *obj );
trp_obj_t *trp_raw_cat( trp_raw_t *obj, va_list args );
uns8b trp_raw_close( trp_raw_t *obj );
trp_obj_t *trp_raw( trp_obj_t *n );
uns8b trp_raw_realloc( trp_obj_t *obj, trp_obj_t *n );
trp_obj_t *trp_raw_mode( trp_obj_t *obj );
trp_obj_t *trp_raw_compression_level( trp_obj_t *obj );
trp_obj_t *trp_raw_uncompressed_len( trp_obj_t *obj );
trp_obj_t *trp_raw_uncompressed_type( trp_obj_t *obj );
trp_obj_t *trp_compress( trp_obj_t *obj, trp_obj_t *level );
trp_obj_t *trp_uncompress( trp_obj_t *obj );
trp_obj_t *trp_raw_read( trp_obj_t *raw, trp_obj_t *stream, trp_obj_t *cnt );
trp_obj_t *trp_raw_write( trp_obj_t *raw, trp_obj_t *stream, trp_obj_t *cnt );
trp_obj_t *trp_raw2str( trp_obj_t *raw, trp_obj_t *cnt );
trp_obj_t *trp_raw_load( trp_obj_t *path );
trp_obj_t *trp_raw_cmp( trp_obj_t *raw1, trp_obj_t *raw2, trp_obj_t *cnt );
uns8b trp_raw_swap( trp_obj_t *raw );

uns8b trp_char_print( trp_print_t *p, trp_char_t *obj );
uns32b trp_char_size( trp_char_t *obj );
void trp_char_encode( trp_char_t *obj, uns8b **buf );
trp_obj_t *trp_char_decode( uns8b **buf );
trp_obj_t *trp_char_equal( trp_char_t *o1, trp_char_t *o2 );
trp_obj_t *trp_char_less( trp_char_t *o1, trp_char_t *o2 );
trp_obj_t *trp_char_length( trp_char_t *obj );
trp_obj_t *trp_char_cat( trp_char_t *c, va_list args );
trp_obj_t *trp_char( uns8b c );
trp_obj_t *trp_nl();
trp_obj_t *trp_int2char( trp_obj_t *obj );

uns8b trp_date_print( trp_print_t *p, trp_date_t *obj );
uns32b trp_date_size( trp_date_t *obj );
void trp_date_encode( trp_date_t *obj, uns8b **buf );
trp_obj_t *trp_date_decode( uns8b **buf );
trp_obj_t *trp_date_equal( trp_date_t *o1, trp_date_t *o2 );
trp_obj_t *trp_date_less( trp_date_t *o1, trp_date_t *o2 );
trp_obj_t *trp_date_length( trp_date_t *obj );
trp_obj_t *trp_date( trp_obj_t *aa, trp_obj_t *mm, trp_obj_t *gg, trp_obj_t *o, trp_obj_t *m, trp_obj_t *s, trp_obj_t *resto, trp_obj_t *tz );
trp_obj_t *trp_date_now();
trp_obj_t *trp_date_cat( trp_date_t *d, va_list args );
trp_obj_t *trp_date_timezone( trp_obj_t *d );
trp_obj_t *trp_date_change_timezone( trp_obj_t *d, trp_obj_t *new_tz );
#define trp_date_gmt(d) trp_date_change_timezone(d,ZERO)
trp_obj_t *trp_date_arpa( trp_obj_t *d );
trp_obj_t *trp_date_ctime( trp_obj_t *d );
trp_obj_t *trp_date_year( trp_obj_t *d );
trp_obj_t *trp_date_month( trp_obj_t *d );
trp_obj_t *trp_date_day( trp_obj_t *d );
trp_obj_t *trp_date_hours( trp_obj_t *d );
trp_obj_t *trp_date_minutes( trp_obj_t *d );
trp_obj_t *trp_date_seconds( trp_obj_t *d );
trp_obj_t *trp_date_usec( trp_obj_t *d );
trp_obj_t *trp_date_wday( trp_obj_t *d );
trp_obj_t *trp_date_s2hhmmss( trp_obj_t *s );
trp_obj_t *trp_date_cal( time_t t );

uns8b trp_file_print( trp_print_t *p, trp_file_t *obj );
trp_obj_t *trp_file_equal( trp_file_t *o1, trp_file_t *o2 );
trp_obj_t *trp_file_length( trp_file_t *obj );
uns8b trp_file_close( trp_file_t *obj );
trp_obj_t *trp_stdin();
trp_obj_t *trp_stdout();
trp_obj_t *trp_stderr();
FILE *trp_file_readable_fp( trp_obj_t *stream );
FILE *trp_file_writable_fp( trp_obj_t *stream );
trp_obj_t *trp_file_openro( trp_obj_t *path );
trp_obj_t *trp_file_openrw( trp_obj_t *path );
trp_obj_t *trp_file_create( trp_obj_t *path );
trp_obj_t *trp_file_open_client( trp_obj_t *server, trp_obj_t *port );
trp_obj_t *trp_file_popenr( trp_obj_t *cmd, ... );
trp_obj_t *trp_file_popenw( trp_obj_t *cmd, ... );
uns8b trp_file_flush( trp_obj_t *stream );
uns8b trp_file_set_pos( trp_obj_t *pos, trp_obj_t *obj );
trp_obj_t *trp_file_pos( trp_obj_t *obj );
trp_obj_t *trp_file_pos_line( trp_obj_t *obj );
trp_obj_t *trp_file_md5sum( trp_obj_t *path );
trp_obj_t *trp_file_sha1sum( trp_obj_t *path );
uns32b trp_file_read_chars( FILE *fp, uns8b *buf, uns32b n );
uns32b trp_file_write_chars( FILE *fp, uns8b *buf, uns32b n );
trp_obj_t *trp_read_char( trp_obj_t *stream );
trp_obj_t *trp_read_line( trp_obj_t *stream );
trp_obj_t *trp_read_str( trp_obj_t *stream, trp_obj_t *cnt );
trp_obj_t *trp_read_uint_le( trp_obj_t *stream, trp_obj_t *cnt );
trp_obj_t *trp_read_uint_be( trp_obj_t *stream, trp_obj_t *cnt );
trp_obj_t *trp_read_sint_le( trp_obj_t *stream, trp_obj_t *cnt );
trp_obj_t *trp_read_sint_be( trp_obj_t *stream, trp_obj_t *cnt );
trp_obj_t *trp_read_float_le( trp_obj_t *stream, trp_obj_t *cnt );
trp_obj_t *trp_read_float_be( trp_obj_t *stream, trp_obj_t *cnt );

uns8b trp_sig64_print( trp_print_t *p, trp_sig64_t *obj );
uns8b trp_mpi_print( trp_print_t *p, trp_mpi_t *obj );
uns8b trp_ratio_print( trp_print_t *p, trp_ratio_t *obj );
uns8b trp_complex_print( trp_print_t *p, trp_complex_t *obj );
uns32b trp_sig64_size( trp_sig64_t *obj );
uns32b trp_mpi_size( trp_mpi_t *obj );
uns32b trp_ratio_size( trp_ratio_t *obj );
uns32b trp_complex_size( trp_complex_t *obj );
void trp_sig64_encode( trp_sig64_t *obj, uns8b **buf );
void trp_mpi_encode( trp_mpi_t *obj, uns8b **buf );
void trp_ratio_encode( trp_ratio_t *obj, uns8b **buf );
void trp_complex_encode( trp_complex_t *obj, uns8b **buf );
trp_obj_t *trp_sig64_decode( uns8b **buf );
trp_obj_t *trp_mpi_decode( uns8b **buf );
trp_obj_t *trp_ratio_decode( uns8b **buf );
trp_obj_t *trp_complex_decode( uns8b **buf );
trp_obj_t *trp_sig64_equal( trp_sig64_t *o1, trp_sig64_t *o2 );
trp_obj_t *trp_mpi_equal( trp_mpi_t *o1, trp_mpi_t *o2 );
trp_obj_t *trp_ratio_equal( trp_ratio_t *o1, trp_ratio_t *o2 );
trp_obj_t *trp_complex_equal( trp_complex_t *o1, trp_complex_t *o2 );
trp_obj_t *trp_sig64_less( trp_sig64_t *o1, trp_sig64_t *o2 );
trp_obj_t *trp_mpi_less( trp_mpi_t *o1, trp_mpi_t *o2 );
trp_obj_t *trp_ratio_less( trp_ratio_t *o1, trp_ratio_t *o2 );
trp_obj_t *trp_complex_less( trp_complex_t *o1, trp_complex_t *o2 );
trp_obj_t *trp_math_less( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_sig64_length( trp_sig64_t *obj );
trp_obj_t *trp_mpi_length( trp_mpi_t *obj );
trp_obj_t *trp_ratio_length( trp_ratio_t *obj );
trp_obj_t *trp_complex_length( trp_complex_t *obj );
uns8b trp_math_set_prec( trp_obj_t *obj );
trp_obj_t *trp_math_get_prec();
uns8b trp_math_set_seed( trp_obj_t *obj );
trp_obj_t *trp_math_get_seed();
trp_obj_t *trp_math_gmp_version();
trp_obj_t *trp_sig64( sig64b val );
trp_obj_t *trp_double( flt64b val );
trp_obj_t *trp_complex( trp_obj_t *re, trp_obj_t *im );
trp_obj_t *trp_zero();
trp_obj_t *trp_uno();
trp_obj_t *trp_dieci();
trp_obj_t *trp_maxint();
trp_obj_t *trp_minint();
trp_obj_t *trp_math_approximate( trp_obj_t *obj );
trp_obj_t *trp_math_num( trp_obj_t *obj );
trp_obj_t *trp_math_den( trp_obj_t *obj );
trp_obj_t *trp_math_re( trp_obj_t *obj );
trp_obj_t *trp_math_im( trp_obj_t *obj );
trp_obj_t *trp_math_floor( trp_obj_t *obj );
trp_obj_t *trp_math_ceil( trp_obj_t *obj );
trp_obj_t *trp_math_rint( trp_obj_t *obj );
#define trp_math_abs(obj) trp_length(obj)
trp_obj_t *trp_math_gcd( trp_obj_t *obj, ... );
trp_obj_t *trp_math_lcm( trp_obj_t *obj, ... );
trp_obj_t *trp_math_fac( trp_obj_t *obj );
trp_obj_t *trp_math_mfac( trp_obj_t *obj, trp_obj_t *mobj );
trp_obj_t *trp_math_primorial( trp_obj_t *obj );
trp_obj_t *trp_math_bin( trp_obj_t *objn, trp_obj_t *objk );
trp_obj_t *trp_math_fib( trp_obj_t *obj );
trp_obj_t *trp_math_lucnum( trp_obj_t *obj );
trp_obj_t *trp_math_probab_isprime( trp_obj_t *obj, trp_obj_t *reps );
trp_obj_t *trp_math_isprime( trp_obj_t *obj );
trp_obj_t *trp_math_nextprime( trp_obj_t *obj );
trp_obj_t *trp_math_perfect_power( trp_obj_t *obj );
trp_obj_t *trp_math_perfect_square( trp_obj_t *obj );
trp_obj_t *trp_math_random( trp_obj_t *obj );
trp_obj_t *trp_math_cat( trp_obj_t *obj, va_list args );
trp_obj_t *trp_math_minus( trp_obj_t *obj, ... );
trp_obj_t *trp_math_times( trp_obj_t *obj, ... );
trp_obj_t *trp_math_ratio( trp_obj_t *obj, ... );
trp_obj_t *trp_math_div( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_math_mod( trp_obj_t *o1, trp_obj_t *o2 );
trp_obj_t *trp_math_sqrt( trp_obj_t *obj );
trp_obj_t *trp_math_pow( trp_obj_t *n, trp_obj_t *m );
trp_obj_t *trp_math_powm( trp_obj_t *n, trp_obj_t *m, trp_obj_t *mod );
trp_obj_t *trp_math_exp( trp_obj_t *n );
trp_obj_t *trp_math_ln( trp_obj_t *n );
trp_obj_t *trp_math_log( trp_obj_t *base, trp_obj_t *n );
trp_obj_t *trp_math_atan( trp_obj_t *n );
trp_obj_t *trp_math_asin( trp_obj_t *n );
trp_obj_t *trp_math_acos( trp_obj_t *n );
trp_obj_t *trp_math_tan( trp_obj_t *n );
trp_obj_t *trp_math_sin( trp_obj_t *n );
trp_obj_t *trp_math_cos( trp_obj_t *n );
trp_obj_t *trp_math_lyapunov( trp_obj_t *seq, trp_obj_t *a, trp_obj_t *b, trp_obj_t *iter );

uns8b trp_list_print( trp_print_t *p, trp_cons_t *obj );
uns32b trp_list_size( trp_cons_t *obj );
void trp_list_encode( trp_cons_t *obj, uns8b **buf );
trp_obj_t *trp_list_decode( uns8b **buf );
trp_obj_t *trp_list_equal( trp_cons_t *o1, trp_cons_t *o2 );
trp_obj_t *trp_list_less( trp_cons_t *o1, trp_cons_t *o2 );
trp_obj_t *trp_list_length( trp_cons_t *obj );
trp_obj_t *trp_list_nth( uns32b n, trp_cons_t *obj );
trp_obj_t *trp_list_sub( uns32b start, uns32b len, trp_cons_t *obj );
trp_obj_t *trp_list_cat( trp_obj_t *obj, va_list args );
uns8b trp_list_in( trp_obj_t *obj, trp_cons_t *seq, uns32b *pos, uns32b nth );
trp_obj_t *trp_cons( trp_obj_t *car, trp_obj_t *cdr );
trp_obj_t *trp_car( trp_obj_t *obj );
trp_obj_t *trp_cdr( trp_obj_t *obj );
trp_obj_t *trp_list( trp_obj_t *obj, ... );
trp_obj_t *trp_list_reverse( trp_obj_t *obj );

uns8b trp_array_print( trp_print_t *p, trp_array_t *obj );
uns32b trp_array_size( trp_array_t *obj );
void trp_array_encode( trp_array_t *obj, uns8b **buf );
trp_obj_t *trp_array_decode( uns8b **buf );
trp_obj_t *trp_array_equal( trp_array_t *o1, trp_array_t *o2 );
trp_obj_t *trp_array_less( trp_array_t *o1, trp_array_t *o2 );
uns8b trp_array_close( trp_array_t *obj );
trp_obj_t *trp_array_length( trp_array_t *obj );
trp_obj_t *trp_array_nth( uns32b n, trp_array_t *obj );
trp_obj_t *trp_array_sub( uns32b start, uns32b len, trp_array_t *obj );
uns8b trp_array_in( trp_obj_t *obj, trp_array_t *seq, uns32b *pos, uns32b nth );
trp_obj_t *trp_array_ext_internal( trp_obj_t *default_val, uns32b incr, uns32b len );
trp_obj_t *trp_array_ext( trp_obj_t *default_val, trp_obj_t *incr, trp_obj_t *len );
trp_obj_t *trp_array_multi( trp_obj_t *default_val, trp_obj_t *len, ... );
uns8b trp_array_insert( trp_obj_t *a, trp_obj_t *pos, trp_obj_t *obj, ... );
uns8b trp_array_remove( trp_obj_t *a, trp_obj_t *pos, trp_obj_t *cnt );
uns8b trp_array_set( trp_obj_t *a, trp_obj_t *pos, trp_obj_t *obj );
uns8b trp_array_set_multi( trp_obj_t *a, trp_obj_t *pos, ... );
uns8b trp_array_inc_multi( trp_obj_t *a, trp_obj_t *pos, ... );
uns8b trp_array_dec_multi( trp_obj_t *a, trp_obj_t *pos, ... );
uns8b trp_array_sort( trp_obj_t *a, trp_obj_t *cmp );
uns8b trp_array_quicksort( trp_obj_t *a, trp_obj_t *cmp );
uns8b trp_array_heapsort( trp_obj_t *a, trp_obj_t *cmp );
uns8b trp_array_mergesort( trp_obj_t *a, trp_obj_t *cmp );

uns8b trp_queue_print( trp_print_t *p, trp_queue_t *obj );
uns32b trp_queue_size( trp_queue_t *obj );
void trp_queue_encode( trp_queue_t *obj, uns8b **buf );
trp_obj_t *trp_queue_decode( uns8b **buf );
trp_obj_t *trp_queue_equal( trp_queue_t *o1, trp_queue_t *o2 );
trp_obj_t *trp_queue_less( trp_queue_t *o1, trp_queue_t *o2 );
uns8b trp_queue_close( trp_queue_t *obj );
trp_obj_t *trp_queue_length( trp_queue_t *obj );
trp_obj_t *trp_queue_nth( uns32b n, trp_queue_t *obj );
uns8b trp_queue_in( trp_obj_t *obj, trp_queue_t *seq, uns32b *pos, uns32b nth );
trp_obj_t *trp_queue();
uns8b trp_queue_put( trp_obj_t *queue, trp_obj_t *obj );
trp_obj_t *trp_queue_get( trp_obj_t *queue );
uns8b trp_queue_swap( trp_obj_t *obj,  trp_obj_t *i,  trp_obj_t *j );

uns8b trp_stack_print( trp_print_t *p, trp_stack_t *obj );
uns32b trp_stack_size( trp_stack_t *obj );
void trp_stack_encode( trp_stack_t *obj, uns8b **buf );
trp_obj_t *trp_stack_decode( uns8b **buf );
trp_obj_t *trp_stack_equal( trp_stack_t *o1, trp_stack_t *o2 );
trp_obj_t *trp_stack_length( trp_stack_t *obj );
trp_obj_t *trp_stack_nth( uns32b n, trp_stack_t *obj );
uns8b trp_stack_in( trp_obj_t *obj, trp_stack_t *seq, uns32b *pos, uns32b nth );
trp_obj_t *trp_stack();
uns8b trp_stack_push( trp_obj_t *stack, trp_obj_t *obj );
trp_obj_t *trp_stack_pop( trp_obj_t *stack );

uns8b trp_cord_print( trp_print_t *p, trp_cord_t *obj );
uns32b trp_cord_size( trp_cord_t *obj );
void trp_cord_encode( trp_cord_t *obj, uns8b **buf );
trp_obj_t *trp_cord_decode( uns8b **buf );
trp_obj_t *trp_cord_equal( trp_cord_t *o1, trp_cord_t *o2 );
trp_obj_t *trp_cord_less( trp_cord_t *o1, trp_cord_t *o2 );
trp_obj_t *trp_cord_length( trp_cord_t *obj );
trp_obj_t *trp_cord_nth( uns32b n, trp_cord_t *obj );
trp_obj_t *trp_cord_sub( uns32b start, uns32b len, trp_cord_t *obj );
trp_obj_t *trp_cord_cat( trp_cord_t *obj, va_list args );
uns8b trp_cord_in( trp_obj_t *obj, trp_cord_t *seq, uns32b *pos, uns32b nth );
trp_obj_t *trp_cord_reverse( trp_cord_t * obj );
trp_obj_t *trp_cord_cons( CORD c, uns32b len );
trp_obj_t *trp_cord( const uns8b *s );
trp_obj_t *trp_cord_empty();
trp_obj_t *trp_cord_explode( trp_obj_t *obj );
trp_obj_t *trp_cord_utf8_length( trp_obj_t *obj );
trp_obj_t *trp_cord_iso2utf8( trp_obj_t *obj );
trp_obj_t *trp_cord_utf82iso( trp_obj_t *obj );
trp_obj_t *trp_cord_koi8_r2utf8( trp_obj_t *obj );
trp_obj_t *trp_cord_greek2utf8( trp_obj_t *obj );
trp_obj_t *trp_cord_windows12522utf8( trp_obj_t *obj );
trp_obj_t *trp_cord_str2num( trp_obj_t *obj );
trp_obj_t *trp_cord_search_func( uns8b flags, trp_obj_t *obj, trp_obj_t *s );
uns8b trp_cord_search_test( uns8b flags, trp_obj_t *obj, trp_obj_t *s, trp_obj_t **pos, trp_obj_t *nth );
trp_obj_t *trp_cord_lmatch_func( uns8b ignore_case, trp_obj_t *o, ... );
trp_obj_t *trp_cord_rmatch_func( uns8b ignore_case, trp_obj_t *o, ... );
uns8b trp_cord_match_test( uns8b flags, trp_obj_t **which, trp_obj_t **which_idx, trp_obj_t **o, ... );
trp_obj_t *trp_cord_ltrim( trp_obj_t *s, ... );
trp_obj_t *trp_cord_rtrim( trp_obj_t *s, ... );
uns8b trp_cord_ltrim_test( trp_obj_t **s, ... );
uns8b trp_cord_rtrim_test( trp_obj_t **s, ... );
trp_obj_t *trp_cord_max_prefix( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_max_prefix_case( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_max_suffix( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_max_suffix_case( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_load( trp_obj_t *path );
trp_obj_t *trp_cord_tile( trp_obj_t *len, ... );
sig32b trp_cord_utf8_next( uns8b *p );
trp_obj_t *trp_cord_utf8_tile( trp_obj_t *len, ... );
trp_obj_t *trp_cord_utf8_head( trp_obj_t *s, trp_obj_t *len );
trp_obj_t *trp_cord_utf8_max_valid_prefix( trp_obj_t *s );
trp_obj_t *trp_cord_utf8_toupper( trp_obj_t *s, ... );
trp_obj_t *trp_cord_utf8_tolower( trp_obj_t *s, ... );
trp_obj_t *trp_cord_subsequencep( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_circular_eq( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_hamming_distance( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_edit_distance( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_protein_weight( trp_obj_t *s );
trp_obj_t *trp_cord_weight2amino( trp_obj_t *weight );
trp_obj_t *trp_cord_alignment_score( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_alignment_score_affine( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_lcs( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_lcs_length( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_edit_alignment( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_cord_global_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_global_alignment_affine( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_fitting_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_global_alignment_score( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_local_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_local_alignment_affine( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_semiglobal_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix );
trp_obj_t *trp_cord_overlap_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix );

uns8b trp_tree_print( trp_print_t *p, trp_tree_t *obj );
uns32b trp_tree_size( trp_tree_t *obj );
void trp_tree_encode( trp_tree_t *obj, uns8b **buf );
trp_obj_t *trp_tree_decode( uns8b **buf );
trp_obj_t *trp_tree_equal( trp_tree_t *o1, trp_tree_t *o2 );
trp_obj_t *trp_tree_less( trp_tree_t *o1, trp_tree_t *o2 );
trp_obj_t *trp_tree_length( trp_tree_t *t );
trp_obj_t *trp_tree_nth( uns32b n, trp_tree_t *obj );
trp_obj_t *trp_tree( trp_obj_t *val, ... );
trp_obj_t *trp_tree_get( trp_obj_t *obj );
trp_obj_t *trp_tree_root( trp_obj_t *obj );
trp_obj_t *trp_tree_level( trp_obj_t *obj );
trp_obj_t *trp_tree_parent( trp_obj_t *obj );
trp_obj_t *trp_tree_children( trp_obj_t *obj );
trp_obj_t *trp_tree_pos( trp_obj_t *obj );
trp_obj_t *trp_tree_node_cnt( trp_obj_t *obj );
uns8b trp_tree_set( trp_obj_t *obj, trp_obj_t *val );
uns8b trp_tree_insert( trp_obj_t *obj, trp_obj_t *pos, trp_obj_t *child );
uns8b trp_tree_append( trp_obj_t *obj, trp_obj_t *child );
uns8b trp_tree_detach( trp_obj_t *obj, trp_obj_t *pos );
uns8b trp_tree_replace( trp_obj_t *obj, trp_obj_t *pos, trp_obj_t *new_obj );
uns8b trp_tree_swap( trp_obj_t *obj, trp_obj_t *pos1, trp_obj_t *pos2 );

uns8b trp_dgraph_print( trp_print_t *p, trp_dgraph_t *obj );
uns32b trp_dgraph_size( trp_dgraph_t *obj );
void trp_dgraph_encode( trp_dgraph_t *obj, uns8b **buf );
trp_obj_t *trp_dgraph_decode( uns8b **buf );
trp_obj_t *trp_dgraph_length( trp_dgraph_t *obj );
trp_obj_t *trp_dgraph();
trp_obj_t *trp_dgraph_first( trp_obj_t *g );
trp_obj_t *trp_dgraph_queue( trp_obj_t *g );
trp_obj_t *trp_dgraph_queue_out( trp_obj_t *n );
trp_obj_t *trp_dgraph_queue_in( trp_obj_t *n );
trp_obj_t *trp_dgraph_dgraph( trp_obj_t *n );
trp_obj_t *trp_dgraph_node( trp_obj_t *g, trp_obj_t *val, trp_obj_t *name );
trp_obj_t *trp_dgraph_is_node( trp_obj_t *n );
trp_obj_t *trp_dgraph_out_cnt( trp_obj_t *n );
trp_obj_t *trp_dgraph_in_cnt( trp_obj_t *n );
trp_obj_t *trp_dgraph_get_val( trp_obj_t *n );
uns8b trp_dgraph_set_val( trp_obj_t *n, trp_obj_t *val );
trp_obj_t *trp_dgraph_get_name( trp_obj_t *n );
uns8b trp_dgraph_set_name( trp_obj_t *n, trp_obj_t *name );
trp_obj_t *trp_dgraph_get_node( trp_obj_t *g, trp_obj_t *name );
trp_obj_t *trp_dgraph_get_id( trp_obj_t *n );
trp_obj_t *trp_dgraph_get_node_by_id( trp_obj_t *g, trp_obj_t *id );
uns8b trp_dgraph_link( trp_obj_t *n1, trp_obj_t *n2, trp_obj_t *val );
uns8b trp_dgraph_link_acyclic( trp_obj_t *n1, trp_obj_t *n2, trp_obj_t *val );
trp_obj_t *trp_dgraph_can_reach( trp_obj_t *n1, trp_obj_t *n2 );
trp_obj_t *trp_dgraph_get_link_val( trp_obj_t *n1, trp_obj_t *n2 );
uns8b trp_dgraph_set_link_val( trp_obj_t *n1, trp_obj_t *n2, trp_obj_t *val );
uns8b trp_dgraph_detach( trp_obj_t *n1, trp_obj_t *n2 );
trp_obj_t *trp_dgraph_succ_cnt( trp_obj_t *n );
trp_obj_t *trp_dgraph_pred_cnt( trp_obj_t *n );
trp_obj_t *trp_dgraph_root_if_is_tree( trp_obj_t *g );
trp_obj_t *trp_dgraph_is_tree( trp_obj_t *g );
trp_obj_t *trp_dgraph_queue_succ( trp_obj_t *n );
trp_obj_t *trp_dgraph_queue_pred( trp_obj_t *n );
trp_obj_t *trp_dgraph_connected_cnt( trp_obj_t *n );
trp_obj_t *trp_dgraph_is_connected( trp_obj_t *g );
trp_obj_t *trp_dgraph_is_acyclic( trp_obj_t *g );

uns8b trp_assoc_print( trp_print_t *p, trp_assoc_t *obj );
uns32b trp_assoc_size( trp_assoc_t *obj );
void trp_assoc_encode( trp_assoc_t *obj, uns8b **buf );
trp_obj_t *trp_assoc_decode( uns8b **buf );
trp_obj_t *trp_assoc_equal( trp_assoc_t *o1, trp_assoc_t *o2 );
trp_obj_t *trp_assoc_length( trp_assoc_t *obj );
trp_obj_t *trp_assoc_height( trp_assoc_t *obj );
uns8b trp_assoc_in( trp_obj_t *obj, trp_assoc_t *seq, uns32b *pos, uns32b nth );
trp_obj_t *trp_assoc();
uns8b trp_assoc_set( trp_obj_t *obj, trp_obj_t *key, trp_obj_t *val );
uns8b trp_assoc_inc( trp_obj_t *obj, trp_obj_t *key, trp_obj_t *val );
uns8b trp_assoc_clr( trp_obj_t *obj, trp_obj_t *key );
trp_obj_t *trp_assoc_get( trp_obj_t *obj, trp_obj_t *key );
trp_obj_t *trp_assoc_queue( trp_obj_t *obj );
trp_obj_t *trp_assoc_root( trp_obj_t *obj );

uns8b trp_set_print( trp_print_t *p, trp_set_t *obj );
uns32b trp_set_size( trp_set_t *obj );
void trp_set_encode( trp_set_t *obj, uns8b **buf );
trp_obj_t *trp_set_decode( uns8b **buf );
trp_obj_t *trp_set_equal( trp_set_t *o1, trp_set_t *o2 );
trp_obj_t *trp_set_length( trp_set_t *obj );
uns8b trp_set_in( trp_obj_t *obj, trp_set_t *seq, uns32b *pos, uns32b nth );
trp_obj_t *trp_set_cat( trp_set_t *obj, va_list args );
trp_obj_t *trp_set( trp_obj_t *x, ... );
uns8b trp_set_insert( trp_obj_t *s, ... );
trp_obj_t *trp_set_queue( trp_obj_t *s );
uns8b trp_set_remove( trp_obj_t *s, trp_obj_t *x );
trp_obj_t *trp_set_intersection( trp_obj_t *s, ... );
trp_obj_t *trp_set_difference( trp_obj_t *s1, trp_obj_t *s2 );
trp_obj_t *trp_set_are_disjoint( trp_obj_t *s1, trp_obj_t *s2 );

uns8b trp_funptr_print( trp_print_t *p, trp_funptr_t *obj );
trp_obj_t *trp_funptr_equal( trp_funptr_t *o1, trp_funptr_t *o2 );
trp_obj_t *trp_funptr_less( trp_funptr_t *o1, trp_funptr_t *o2 );
trp_obj_t *trp_funptr_length( trp_funptr_t *fptr );
trp_obj_t *trp_funptr( objfun_t f, uns8b nargs, trp_obj_t *name );
trp_obj_t *trp_funptr_equal_obj();
trp_obj_t *trp_funptr_less_obj();
trp_obj_t *trp_funptr_call( trp_obj_t *f, ... );

uns8b trp_netptr_print( trp_print_t *p, trp_funptr_t *obj );
trp_obj_t *trp_netptr_equal( trp_funptr_t *o1, trp_funptr_t *o2 );
trp_obj_t *trp_netptr_less( trp_funptr_t *o1, trp_funptr_t *o2 );
trp_obj_t *trp_netptr_length( trp_funptr_t *fptr );
trp_obj_t *trp_netptr( uns8bfun_t f, uns8b nargs, trp_obj_t *name );
uns8b trp_netptr_call( trp_obj_t *nptr, ... );

uns8b trp_regex_close( trp_regex_t *obj );
trp_obj_t *trp_regex_length( trp_regex_t *obj );
trp_obj_t *trp_regsearch( trp_obj_t *key, trp_obj_t *txt, trp_obj_t *flags );
trp_obj_t *trp_regcomp( trp_obj_t *obj, trp_obj_t *flags );
trp_obj_t *trp_regexec( trp_obj_t *re, trp_obj_t *txt, trp_obj_t *flags );
uns8b trp_regexec_test( trp_obj_t *re, trp_obj_t *txt );

uns8b trp_fibo_print( trp_print_t *p, trp_fibo_t *obj );
uns32b trp_fibo_size( trp_fibo_t *obj );
void trp_fibo_encode( trp_fibo_t *obj, uns8b **buf );
trp_obj_t *trp_fibo_decode( uns8b **buf );
trp_obj_t *trp_fibo_length( trp_fibo_t *obj );
trp_obj_t *trp_fibo_width( trp_fibo_node_t *obj );
trp_obj_t *trp_fibo( trp_obj_t *cmp );
trp_obj_t *trp_fibo_queue( trp_obj_t *h );
trp_obj_t *trp_fibo_first( trp_obj_t *h );
trp_obj_t *trp_fibo_fibo( trp_obj_t *x );
trp_obj_t *trp_fibo_get_key( trp_obj_t *x );
trp_obj_t *trp_fibo_get_obj( trp_obj_t *x );
trp_obj_t *trp_fibo_insert( trp_obj_t *h, trp_obj_t *key, trp_obj_t *obj );
trp_obj_t *trp_fibo_extract( trp_obj_t *h );
uns8b trp_fibo_decrease_key( trp_obj_t *x, trp_obj_t *key );
uns8b trp_fibo_delete( trp_obj_t *x );
uns8b trp_fibo_set_key( trp_obj_t *x, trp_obj_t *key );
uns8b trp_fibo_set_obj( trp_obj_t *x, trp_obj_t *obj );
uns8b trp_fibo_merge( trp_obj_t *h1, trp_obj_t *h2 );

#endif /* !__trp__h */

/*
    TreeP Run Time Support
    Copyright (C) 2008-2026 Frank Sinapsi

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

#include "../trp/trp.h"
#include "./trpgcrypt.h"
#include "../trppix/trppix_internal.h"

extern uns32b trp_size_internal( trp_obj_t *obj );
extern void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );

#ifdef TRP_BIG_ENDIAN
static uns8b trp_gcry_luby_rackoff_md_initialize( int flags, gcry_md_hd_t *hd, int algo, char *pass );
static uns32b trp_gcry_luby_rackoff( uns32b len, gcry_md_hd_t *hd, uns32b index );
static uns32b trp_gcry_luby_rackoff_inv( uns32b len, gcry_md_hd_t *hd, uns32b index );
static trp_obj_t *trp_gcry_permute_basic( trp_obj_t *size, trp_obj_t *pass_phrase, trp_obj_t *index, uns32bfun_t fun );
static trp_obj_t *trp_gcry_stego_extract_new( gcry_md_hd_t *hd, uns8b *map, uns32b cnt_len, uns32b l );
static trp_obj_t *trp_gcry_stego_extract_old( gcry_md_hd_t *hd, uns8b *map, uns32b cnt_len, uns32b l );
#else
#include "./md5.h"
static void trp_gcry_luby_rackoff_md_initialize( struct md5_ctx *context, char *pass );
static uns32b trp_gcry_luby_rackoff( uns32b len, struct md5_ctx *context, uns32b index );
static uns32b trp_gcry_luby_rackoff_inv( uns32b len, struct md5_ctx *context, uns32b index );
static trp_obj_t *trp_gcry_stego_extract_new( struct md5_ctx *context, uns8b *map, uns32b cnt_len, uns32b l );
static trp_obj_t *trp_gcry_stego_extract_old( struct md5_ctx *context, uns8b *map, uns32b cnt_len, uns32b l );
#endif
static trp_raw_t *trp_gcry_md_hash_buffer( uns8b encode_only_if_needed, int algo, trp_obj_t *obj );
static trp_raw_t *trp_gcry_md_hash_path( int algo, trp_obj_t *path );
static trp_obj_t *trp_gcry_raw2ascii( trp_raw_t *raw );
static trp_obj_t *trp_gcry_md_hash_basic( uns8b encode_only_if_needed, trp_obj_t *algo, trp_obj_t *obj );
#define trp_gcry_trunc_index(index,length) ((index)&(0x0000ffff>>(16-length)))
#define trp_gcry_stego_inject(c,i,info) if(info)c[(((i)/3)<<2)|((i)%3)]|=1;else c[(((i)/3)<<2)|((i)%3)]&=0xfe
#define trp_gcry_stego_eject(c,i) (c[(((i)/3)<<2)|((i)%3)]&1)
#define trp_gcry_hexdigit(digit) (((digit)<10)?((digit)+'0'):((digit)-10+'a'))

uns8b trp_gcry_init()
{
    if ( !gcry_check_version( GCRYPT_VERSION ) ) {
        fprintf( stderr, "Initialization of libgcrypt failed\n" );
        return 1;
    }

    /* Disable secure memory.  */
    gcry_control( GCRYCTL_DISABLE_SECMEM, 0 );

    /* ... If required, other initialization goes here.  */

    /* Tell Libgcrypt that initialization has completed. */
    gcry_control( GCRYCTL_INITIALIZATION_FINISHED, 0 );
    return 0;
}

void trp_gcry_quit()
{
}

trp_obj_t *trp_gcry_version()
{
    return trp_cord( GCRYPT_VERSION );
}

#ifdef TRP_BIG_ENDIAN

static uns8b trp_gcry_luby_rackoff_md_initialize( int flags, gcry_md_hd_t *hd, int algo, char *pass )
/*
 inizializza 4 context con la pass phrase pass
 */
{
    int i;

    if ( flags & 1 ) {
        gcry_md_reset( hd[ 0 ] );
        gcry_md_reset( hd[ 1 ] );
        gcry_md_reset( hd[ 2 ] );
        gcry_md_reset( hd[ 3 ] );
    } else {
        if ( gcry_md_open( &( hd[ 0 ] ), algo, 0 ) ||
             gcry_md_open( &( hd[ 1 ] ), algo, 0 ) ||
             gcry_md_open( &( hd[ 2 ] ), algo, 0 ) ||
             gcry_md_open( &( hd[ 3 ] ), algo, 0 ) ) {
            gcry_md_close( hd[ 0 ] );
            gcry_md_close( hd[ 1 ] );
            gcry_md_close( hd[ 2 ] );
            gcry_md_close( hd[ 3 ] );
            return 1;
        }
    }
    for ( i = 0 ; *pass ; pass++, i = ( i + 1 ) % 4 )
        gcry_md_putc( hd[ i ], *pass );
    return 0;
}

static uns32b trp_gcry_luby_rackoff( uns32b len, gcry_md_hd_t *hd, uns32b index )
/*
 generatore di permutazioni pseudocasuali di Luby-Rackoff;
 len è la lunghezza in bit degli indici;
 hd è la chiave, già suddivisa in 4 context;
 index è l'indice da rimappare;
 le strutture context non vengono modificate, quindi possono essere
 riutilizzate
 */
{
    uns32b length_L, length_R, L, R, *p;
    gcry_md_hd_t hd_tmp;

    length_L = len >> 1;
    length_R = len - length_L;

    L = trp_gcry_trunc_index( index, length_L );
    R = trp_gcry_trunc_index( index >> length_L, length_R );

    gcry_md_copy( &hd_tmp, hd[ 0 ] );
    gcry_md_putc( hd_tmp, R & 0xff );
    gcry_md_putc( hd_tmp, R >> 8 );
    p = (uns32b *)gcry_md_read( hd_tmp, GCRY_MD_MD5 );
    L = trp_gcry_trunc_index( L ^ *p, length_L );
    gcry_md_close( hd_tmp );

    gcry_md_copy( &hd_tmp, hd[ 1 ] );
    gcry_md_putc( hd_tmp, L & 0xff );
    gcry_md_putc( hd_tmp, L >> 8 );
    p = (uns32b *)gcry_md_read( hd_tmp, GCRY_MD_MD5 );
    R = trp_gcry_trunc_index( R ^ *p, length_R );
    gcry_md_close( hd_tmp );

    gcry_md_copy( &hd_tmp, hd[ 2 ] );
    gcry_md_putc( hd_tmp, R & 0xff );
    gcry_md_putc( hd_tmp, R >> 8 );
    p = (uns32b *)gcry_md_read( hd_tmp, GCRY_MD_MD5 );
    L = trp_gcry_trunc_index( L ^ *p, length_L );
    gcry_md_close( hd_tmp );

    gcry_md_copy( &hd_tmp, hd[ 3 ] );
    gcry_md_putc( hd_tmp, L & 0xff );
    gcry_md_putc( hd_tmp, L >> 8 );
    p = (uns32b *)gcry_md_read( hd_tmp, GCRY_MD_MD5 );
    R = trp_gcry_trunc_index( R ^ *p, length_R );
    gcry_md_close( hd_tmp );

    return ( R << length_L ) | L;
}

static uns32b trp_gcry_luby_rackoff_inv( uns32b len, gcry_md_hd_t *hd, uns32b index )
/*
 genera la permutazione inversa della precedente
 */
{
    uns32b length_L, length_R, L, R, *p;
    gcry_md_hd_t hd_tmp;

    length_L = len >> 1;
    length_R = len - length_L;

    L = trp_gcry_trunc_index( index, length_L );
    R = trp_gcry_trunc_index( index >> length_L, length_R );

    gcry_md_copy( &hd_tmp, hd[ 3 ] );
    gcry_md_putc( hd_tmp, L & 0xff );
    gcry_md_putc( hd_tmp, L >> 8 );
    p = (uns32b *)gcry_md_read( hd_tmp, GCRY_MD_MD5 );
    R = trp_gcry_trunc_index( R ^ *p, length_R );
    gcry_md_close( hd_tmp );

    gcry_md_copy( &hd_tmp, hd[ 2 ] );
    gcry_md_putc( hd_tmp, R & 0xff );
    gcry_md_putc( hd_tmp, R >> 8 );
    p = (uns32b *)gcry_md_read( hd_tmp, GCRY_MD_MD5 );
    L = trp_gcry_trunc_index( L ^ *p, length_L );
    gcry_md_close( hd_tmp );

    gcry_md_copy( &hd_tmp, hd[ 1 ] );
    gcry_md_putc( hd_tmp, L & 0xff );
    gcry_md_putc( hd_tmp, L >> 8 );
    p = (uns32b *)gcry_md_read( hd_tmp, GCRY_MD_MD5 );
    R = trp_gcry_trunc_index( R ^ *p, length_R );
    gcry_md_close( hd_tmp );

    gcry_md_copy( &hd_tmp, hd[ 0 ] );
    gcry_md_putc( hd_tmp, R & 0xff );
    gcry_md_putc( hd_tmp, R >> 8 );
    p = (uns32b *)gcry_md_read( hd_tmp, GCRY_MD_MD5 );
    L = trp_gcry_trunc_index( L ^ *p, length_L );
    gcry_md_close( hd_tmp );

    return ( R << length_L ) | L;
}

static trp_obj_t *trp_gcry_permute_basic( trp_obj_t *size, trp_obj_t *pass_phrase, trp_obj_t *index, uns32bfun_t fun )
/*
 dato l'indice index, rende l'indice ad esso associato dalla permutazione
 di Luby-Rackoff generata dalla chiave pass_phrase (una stringa di lunghezza arbitraria);
 size è la dimensione dello spazio degli indici
 */
{
    static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    static trp_obj_t *gcry_pass_phrase = NULL;
    static gcry_md_hd_t hd[ 4 ];
    uns32b ssize, iindex, l, m;

    if ( trp_cast_uns32b( size, &ssize ) || trp_cast_uns32b( index, &iindex ) )
        return UNDEF;
    if ( ( iindex >= ssize ) || ( ssize < 2 ) )
        return UNDEF;
    for ( l = 1, m = 2 ; m < ssize ; l++, m <<= 1 );
    pthread_mutex_lock( &mut );
    if ( pass_phrase != gcry_pass_phrase ) {
        uns8b *pass = trp_csprint( pass_phrase );
        if ( trp_gcry_luby_rackoff_md_initialize( gcry_pass_phrase ? 1 : 0, hd, GCRY_MD_MD5, pass ) ) {
            pthread_mutex_unlock( &mut );
            trp_csprint_free( pass );
            return UNDEF;
        }
        trp_csprint_free( pass );
        gcry_pass_phrase = pass_phrase;
    }
    pthread_mutex_unlock( &mut );
    do iindex = (fun)( l, hd, iindex ); while ( iindex >= ssize );
    return trp_sig64( iindex );
}

trp_obj_t *trp_gcry_permute( trp_obj_t *size, trp_obj_t *pass_phrase, trp_obj_t *index )
{
    return trp_gcry_permute_basic( size, pass_phrase, index, trp_gcry_luby_rackoff );
}

trp_obj_t *trp_gcry_permute_inv( trp_obj_t *size, trp_obj_t *pass_phrase, trp_obj_t *index )
{
    return trp_gcry_permute_basic( size, pass_phrase, index, trp_gcry_luby_rackoff_inv );
}

uns8b trp_gcry_stego_insert( trp_obj_t *obj, trp_obj_t *pass_phrase, trp_obj_t *msg )
{
    uns8b *map, *p;
    uns32b cnt_len, msg_len, l, m, i, j;
    gcry_md_hd_t hd[ 4 ];

    if ( obj->tipo != TRP_PIX )
        return 1;
    if ( ( map = ((trp_pix_t *)obj)->map.p ) == NULL )
        return 1;
    cnt_len = 3 * ((trp_pix_t *)obj)->w * ((trp_pix_t *)obj)->h;
    if ( cnt_len < 32 )
        return 1;
    if ( ( msg = trp_compress( msg, DIECI ) ) == UNDEF )
        return 1;
    msg_len = ( ((trp_raw_t *)msg)->len << 3 );
    if ( msg_len + 88 > cnt_len ) {
        trp_gc_free( ((trp_raw_t *)msg)->data );
        trp_gc_free( msg );
        return 1;
    }
    for ( l = 1, m = 2 ; m < cnt_len ; l++, m <<= 1 );
    p = trp_csprint( pass_phrase );
    if ( trp_gcry_luby_rackoff_md_initialize( 0, hd, GCRY_MD_MD5, p ) ) {
        trp_csprint_free( p );
        return 1;
    }
    trp_csprint_free( p );
    for ( j = 0, m = ((trp_raw_t *)msg)->len ; j < 32 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( j = 32, m = ((trp_raw_t *)msg)->mode ; j < 40 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( j = 40, m = ((trp_raw_t *)msg)->unc_tipo ; j < 48 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( j = 48, m = ((trp_raw_t *)msg)->compression_level ; j < 56 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( j = 56, m = ((trp_raw_t *)msg)->unc_len ; j < 88 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( m = 0, p = ((trp_raw_t *)msg)->data ; m < msg_len ; m++ ) {
        i = m + 88;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, ( p[ m >> 3 ] >> ( m & 7 ) ) & 1 );
    }
    trp_gc_free( ((trp_raw_t *)msg)->data );
    trp_gc_free( msg );
    gcry_md_close( hd[ 0 ] );
    gcry_md_close( hd[ 1 ] );
    gcry_md_close( hd[ 2 ] );
    gcry_md_close( hd[ 3 ] );
    return 0;
}

trp_obj_t *trp_gcry_stego_extract( trp_obj_t *obj, trp_obj_t *pass_phrase )
{
    uns8b *map, *p;
    uns32b cnt_len, l, m;
    gcry_md_hd_t hd[ 4 ];

    if ( obj->tipo != TRP_PIX )
        return UNDEF;
    if ( ( map = ((trp_pix_t *)obj)->map.p ) == NULL )
        return UNDEF;
    cnt_len = 3 * ((trp_pix_t *)obj)->w * ((trp_pix_t *)obj)->h;
    if ( cnt_len < 88 )
        return UNDEF;
    for ( l = 1, m = 2 ; m < cnt_len ; l++, m <<= 1 );
    p = trp_csprint( pass_phrase );
    if ( trp_gcry_luby_rackoff_md_initialize( 0, hd, GCRY_MD_MD5, p ) ) {
        trp_csprint_free( p );
        return UNDEF;
    }
    trp_csprint_free( p );
    obj = trp_gcry_stego_extract_new( hd, map, cnt_len, l );
    if ( obj == UNDEF )
        obj = trp_gcry_stego_extract_old( hd, map, cnt_len, l );
    gcry_md_close( hd[ 0 ] );
    gcry_md_close( hd[ 1 ] );
    gcry_md_close( hd[ 2 ] );
    gcry_md_close( hd[ 3 ] );
    return obj;
}

static trp_obj_t *trp_gcry_stego_extract_new( gcry_md_hd_t *hd, uns8b *map, uns32b cnt_len, uns32b l )
{
    trp_obj_t *res;
    trp_raw_t *raw;
    uns8b *p;
    uns32b len, m, i, j, k;

    for ( j = 0, len = 0, m = 1 ; j < 32 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            len |= m;
    }
    if ( ( len == 0 ) || ( len & 0xe0000000 ) || ( ( len << 3 ) + 88 > cnt_len ) )
        return UNDEF;
    if ( ( raw = (trp_raw_t *)trp_raw_internal( len, 0 ) ) == NULL )
        return UNDEF;
    for ( j = 32, k = 0, m = 1 ; j < 40 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->mode = (uns8b)k;
    for ( j = 40, k = 0, m = 1 ; j < 48 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->unc_tipo = (uns8b)k;
    for ( j = 48, k = 0, m = 1 ; j < 56 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->compression_level = (uns8b)k;
    for ( j = 56, k = 0, m = 1 ; j < 88 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->unc_len = k;
    if ( ( raw->mode > 2 ) ||
         ( raw->unc_tipo >= TRP_MAX_T ) ||
         ( raw->compression_level > 9 ) ||
         ( raw->len > raw->unc_len ) ||
         ( ( raw->compression_level == 0 ) && ( raw->len != raw->unc_len ) ) ) {
        trp_gc_free( raw->data );
        trp_gc_free( raw );
        return UNDEF;
    }
    for ( m = 88, p = raw->data ; len ; len--, p++ )
        for ( k = 0, j = 1, *p = 0 ; k < 8 ; k++, m++, j <<= 1 ) {
            i = m;
            do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
            if ( trp_gcry_stego_eject( map, i ) )
                *p |= j;
        }
    res = trp_uncompress( (trp_obj_t *)raw );
    trp_gc_free( raw->data );
    trp_gc_free( raw );
    return res;
}

static trp_obj_t *trp_gcry_stego_extract_old( gcry_md_hd_t *hd, uns8b *map, uns32b cnt_len, uns32b l )
{
    trp_obj_t *res;
    trp_raw_t *raw;
    uns8b *p;
    uns32b msg_len, m, i, j, k;

    cnt_len -= 32;
    for ( j = 0, i = 0, m = 1 ; j < 32 ; j++, m <<= 1 )
        if ( trp_gcry_stego_eject( map, cnt_len + j ) )
            i |= m;
    msg_len = i << 3;
    if ( ( msg_len == 0 ) || ( i & 0xe0000000 ) || ( msg_len + 56 > cnt_len ) )
        return UNDEF;
    if ( ( raw = (trp_raw_t *)trp_raw_internal( i, 0 ) ) == NULL )
        return UNDEF;
    for ( j = 0, k = 0, m = 1 ; j < 8 ; j++, m <<= 1 ) {
        i = msg_len + j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->mode = (uns8b)k;
    for ( j = 8, k = 0, m = 1 ; j < 16 ; j++, m <<= 1 ) {
        i = msg_len + j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->unc_tipo = (uns8b)k;
    for ( j = 16, k = 0, m = 1 ; j < 24 ; j++, m <<= 1 ) {
        i = msg_len + j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->compression_level = (uns8b)k;
    for ( j = 24, k = 0, m = 1 ; j < 56 ; j++, m <<= 1 ) {
        i = msg_len + j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->unc_len = k;
    if ( ( raw->mode > 2 ) ||
         ( raw->unc_tipo >= TRP_MAX_T ) ||
         ( raw->compression_level > 9 ) ||
         ( raw->len > raw->unc_len ) ||
         ( ( raw->compression_level == 0 ) && ( raw->len != raw->unc_len ) ) ) {
        trp_gc_free( raw->data );
        trp_gc_free( raw );
        return UNDEF;
    }
    for ( m = 0, p = raw->data, msg_len >>= 3 ; msg_len ; msg_len--, p++ )
        for ( k = 0, j = 1, *p = 0 ; k < 8 ; k++, m++, j <<= 1 ) {
            i = m;
            do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
            if ( trp_gcry_stego_eject( map, i ) )
                *p |= j;
        }
    res = trp_uncompress( (trp_obj_t *)raw );
    trp_gc_free( raw->data );
    trp_gc_free( raw );
    return res;
}

uns8b trp_gcry_stego_destroy( trp_obj_t *obj, trp_obj_t *pass_phrase )
{
    uns8b *map, *p;
    uns32b cnt_len, len, i, j, k, l, m;
    gcry_md_hd_t hd[ 4 ];

    if ( obj->tipo != TRP_PIX )
        return 1;
    if ( ( map = ((trp_pix_t *)obj)->map.p ) == NULL )
        return 1;
    cnt_len = 3 * ((trp_pix_t *)obj)->w * ((trp_pix_t *)obj)->h;
    if ( cnt_len < 88 )
        return 1;
    for ( l = 1, m = 2 ; m < cnt_len ; l++, m <<= 1 );
    p = trp_csprint( pass_phrase );
    if ( trp_gcry_luby_rackoff_md_initialize( 0, hd, GCRY_MD_MD5, p ) ) {
        trp_csprint_free( p );
        return 1;
    }
    trp_csprint_free( p );
    for ( j = 0, k = 0, m = 1 ; j < 32 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    len = ( k << 3 ) + 88;
    if ( ( len == 0 ) || ( k & 0xe0000000 ) || ( len > cnt_len ) ) {
        /*
         si prova se è compatibile con il vecchio formato
         */
        cnt_len -= 32;
        for ( j = 0, i = 0, m = 1 ; j < 32 ; j++, m <<= 1 )
            if ( trp_gcry_stego_eject( map, cnt_len + j ) )
                i |= m;
        len = ( i << 3 ) + 56;
        if ( ( len == 0 ) || ( i & 0xe0000000 ) || ( len > cnt_len ) ) {
            gcry_md_close( hd[ 0 ] );
            gcry_md_close( hd[ 1 ] );
            gcry_md_close( hd[ 2 ] );
            gcry_md_close( hd[ 3 ] );
            return 1;
        }
        /*
         è compatibile
         */
        pass_phrase = trp_sig64( 0x100000000LL );
        obj = trp_math_random( pass_phrase );
        j = (uns32b)( ((trp_sig64_t *)obj)->val );
        trp_gc_free( obj );
        for ( i = 0 ; i < 32 ; i++, j >>= 1 )
            trp_gcry_stego_inject( map, cnt_len + i, j & 1 );
    } else {
        pass_phrase = trp_sig64( 0x100000000LL );
    }
    for ( m = 0, k = 0 ; m < len ; m++, k--, j >>= 1 ) {
        i = m;
        do i = trp_gcry_luby_rackoff( l, hd, i ); while ( i >= cnt_len );
        if ( k == 0 ) {
            obj = trp_math_random( pass_phrase );
            j = (uns32b)( ((trp_sig64_t *)obj)->val );
            trp_gc_free( obj );
            k = 32;
        }
        trp_gcry_stego_inject( map, i, j & 1 );
    }
    trp_gc_free( pass_phrase );
    gcry_md_close( hd[ 0 ] );
    gcry_md_close( hd[ 1 ] );
    gcry_md_close( hd[ 2 ] );
    gcry_md_close( hd[ 3 ] );
    return 0;
}

#else

static void trp_gcry_luby_rackoff_md_initialize( struct md5_ctx *context, char *pass )
/*
 inizializza 4 strutture context con la pass phrase pass
 */
{
    int i;

    md5_init_ctx( &( context[ 0 ] ) );
    md5_init_ctx( &( context[ 1 ] ) );
    md5_init_ctx( &( context[ 2 ] ) );
    md5_init_ctx( &( context[ 3 ] ) );
    for ( i = 0 ; *pass ; pass++, i = ( i + 1 ) % 4 )
        md5_process_bytes( pass, 1, &( context[ i ] ) );
}

static uns32b trp_gcry_luby_rackoff( uns32b len, struct md5_ctx *context, uns32b index )
/*
 generatore di permutazioni pseudocasuali di Luby-Rackoff;
 len è la lunghezza in bit degli indici;
 context è la chiave, già suddivisa in 4 strutture md5_ctx;
 index è l'indice da rimappare;
 le strutture context non vengono modificate, quindi possono essere
 riutilizzate
 */
{
    uns32b length_L, length_R, L, R, *p;
    uns8b digest[ 16 ];
    struct md5_ctx md;

    p = (uns32b *)digest;
    length_L = len >> 1;
    length_R = len - length_L;

    L = trp_gcry_trunc_index( index, length_L );
    R = trp_gcry_trunc_index( index >> length_L, length_R );

    memcpy( &md, &( context[ 0 ] ), sizeof( struct md5_ctx ) );
    md5_process_bytes( (char *)( &R ), 2, &md );
    md5_finish_ctx( &md, digest );

    L = trp_gcry_trunc_index( L ^ *p, length_L );

    memcpy( &md, &( context[ 1 ] ), sizeof( struct md5_ctx ) );
    md5_process_bytes( (char *)( &L ), 2, &md );
    md5_finish_ctx( &md, digest );

    R = trp_gcry_trunc_index( R ^ *p, length_R );

    memcpy( &md, &( context[ 2 ] ), sizeof( struct md5_ctx ) );
    md5_process_bytes( (char *)( &R ), 2, &md );
    md5_finish_ctx( &md, digest );

    L = trp_gcry_trunc_index( L ^ *p, length_L );

    memcpy( &md, &( context[ 3 ] ), sizeof( struct md5_ctx ) );
    md5_process_bytes( (char *)( &L ), 2, &md );
    md5_finish_ctx( &md, digest );

    R = trp_gcry_trunc_index( R ^ *p, length_R );

    return ( R << length_L ) | L;
}

static uns32b trp_gcry_luby_rackoff_inv( uns32b len, struct md5_ctx *context, uns32b index )
/*
 genera la permutazione inversa della funzione precedente
 */
{
    uns32b length_L, length_R, L, R, *p;
    uns8b digest[ 16 ];
    struct md5_ctx md;

    p = (uns32b *)digest;
    length_L = len >> 1;
    length_R = len - length_L;

    L = trp_gcry_trunc_index( index, length_L );
    R = trp_gcry_trunc_index( index >> length_L, length_R );

    memcpy( &md, &( context[ 3 ] ), sizeof( struct md5_ctx ) );
    md5_process_bytes( (char *)( &L ), 2, &md );
    md5_finish_ctx( &md, digest );

    R = trp_gcry_trunc_index( R ^ *p, length_R );

    memcpy( &md, &( context[ 2 ] ), sizeof( struct md5_ctx ) );
    md5_process_bytes( (char *)( &R ), 2, &md );
    md5_finish_ctx( &md, digest );

    L = trp_gcry_trunc_index( L ^ *p, length_L );

    memcpy( &md, &( context[ 1 ] ), sizeof( struct md5_ctx ) );
    md5_process_bytes( (char *)( &L ), 2, &md );
    md5_finish_ctx( &md, digest );

    R = trp_gcry_trunc_index( R ^ *p, length_R );

    memcpy( &md, &( context[ 0 ] ), sizeof( struct md5_ctx ) );
    md5_process_bytes( (char *)( &R ), 2, &md );
    md5_finish_ctx( &md, digest );

    L = trp_gcry_trunc_index( L ^ *p, length_L );

    return ( R << length_L ) | L;
}

trp_obj_t *trp_gcry_permute( trp_obj_t *size, trp_obj_t *pass_phrase, trp_obj_t *index )
/*
 dato l'indice index, rende l'indice ad esso associato dalla permutazione
 di Luby-Rackoff generata dalla chiave pass_phrase (una stringa di lunghezza arbitraria);
 size è la dimensione dello spazio degli indici
 */
{
    uns32b ssize, iindex, l, m;
    uns8b *pass;
    struct md5_ctx context[ 4 ];

    if ( trp_cast_uns32b( size, &ssize ) || trp_cast_uns32b( index, &iindex ) )
        return UNDEF;
    if ( ( iindex >= ssize ) || ( ssize < 2 ) )
        return UNDEF;
    for ( l = 1, m = 2 ; m < ssize ; l++, m <<= 1 );
    pass = trp_csprint( pass_phrase );
    trp_gcry_luby_rackoff_md_initialize( context, pass );
    trp_csprint_free( pass );
    do iindex = trp_gcry_luby_rackoff( l, context, iindex ); while ( iindex >= ssize );
    return trp_sig64( iindex );
}

trp_obj_t *trp_gcry_permute_inv( trp_obj_t *size, trp_obj_t *pass_phrase, trp_obj_t *index )
/*
 dato l'indice index, rende l'indice ad esso associato dalla permutazione inversa
 di Luby-Rackoff generata dalla chiave pass_phrase (una stringa di lunghezza arbitraria);
 size è la dimensione dello spazio degli indici
 */
{
    uns32b ssize, iindex, l, m;
    uns8b *pass;
    struct md5_ctx context[ 4 ];

    if ( trp_cast_uns32b( size, &ssize ) || trp_cast_uns32b( index, &iindex ) )
        return UNDEF;
    if ( ( iindex >= ssize ) || ( ssize < 2 ) )
        return UNDEF;
    for ( l = 1, m = 2 ; m < ssize ; l++, m <<= 1 );
    pass = trp_csprint( pass_phrase );
    trp_gcry_luby_rackoff_md_initialize( context, pass );
    trp_csprint_free( pass );
    do iindex = trp_gcry_luby_rackoff_inv( l, context, iindex ); while ( iindex >= ssize );
    return trp_sig64( iindex );
}

uns8b trp_gcry_stego_insert( trp_obj_t *obj, trp_obj_t *pass_phrase, trp_obj_t *msg )
{
    uns8b *map, *p;
    uns32b cnt_len, msg_len, l, m, i, j;
    struct md5_ctx context[ 4 ];

    if ( obj->tipo != TRP_PIX )
        return 1;
    if ( ( map = ((trp_pix_t *)obj)->map.p ) == NULL )
        return 1;
    cnt_len = 3 * ((trp_pix_t *)obj)->w * ((trp_pix_t *)obj)->h;
    if ( cnt_len < 32 )
        return 1;
    if ( ( msg = trp_compress( msg, DIECI ) ) == UNDEF )
        return 1;
    msg_len = ( ((trp_raw_t *)msg)->len << 3 );
    if ( msg_len + 88 > cnt_len ) {
        trp_gc_free( ((trp_raw_t *)msg)->data );
        trp_gc_free( msg );
        return 1;
    }
    for ( l = 1, m = 2 ; m < cnt_len ; l++, m <<= 1 );
    p = trp_csprint( pass_phrase );
    trp_gcry_luby_rackoff_md_initialize( context, p );
    trp_csprint_free( p );
    for ( j = 0, m = ((trp_raw_t *)msg)->len ; j < 32 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( j = 32, m = ((trp_raw_t *)msg)->mode ; j < 40 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( j = 40, m = ((trp_raw_t *)msg)->unc_tipo ; j < 48 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( j = 48, m = ((trp_raw_t *)msg)->compression_level ; j < 56 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( j = 56, m = ((trp_raw_t *)msg)->unc_len ; j < 88 ; j++, m >>= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, m & 1 );
    }
    for ( m = 0, p = ((trp_raw_t *)msg)->data ; m < msg_len ; m++ ) {
        i = m + 88;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        trp_gcry_stego_inject( map, i, ( p[ m >> 3 ] >> ( m & 7 ) ) & 1 );
    }
    trp_gc_free( ((trp_raw_t *)msg)->data );
    trp_gc_free( msg );
    return 0;
}

trp_obj_t *trp_gcry_stego_extract( trp_obj_t *obj, trp_obj_t *pass_phrase )
{
    uns8b *map, *p;
    uns32b cnt_len, l, m;
    struct md5_ctx context[ 4 ];

    if ( obj->tipo != TRP_PIX )
        return UNDEF;
    if ( ( map = ((trp_pix_t *)obj)->map.p ) == NULL )
        return UNDEF;
    cnt_len = 3 * ((trp_pix_t *)obj)->w * ((trp_pix_t *)obj)->h;
    if ( cnt_len < 88 )
        return UNDEF;
    for ( l = 1, m = 2 ; m < cnt_len ; l++, m <<= 1 );
    p = trp_csprint( pass_phrase );
    trp_gcry_luby_rackoff_md_initialize( context, p );
    trp_csprint_free( p );
    obj = trp_gcry_stego_extract_new( context, map, cnt_len, l );
    if ( obj == UNDEF )
        obj = trp_gcry_stego_extract_old( context, map, cnt_len, l );
    return obj;
}

static trp_obj_t *trp_gcry_stego_extract_new( struct md5_ctx *context, uns8b *map, uns32b cnt_len, uns32b l )
{
    trp_obj_t *res;
    trp_raw_t *raw;
    uns8b *p;
    uns32b len, m, i, j, k;

    for ( j = 0, len = 0, m = 1 ; j < 32 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            len |= m;
    }
    if ( ( len == 0 ) || ( len & 0xe0000000 ) || ( ( len << 3 ) + 88 > cnt_len ) )
        return UNDEF;
    if ( ( raw = (trp_raw_t *)trp_raw_internal( len, 0 ) ) == NULL )
        return UNDEF;
    for ( j = 32, k = 0, m = 1 ; j < 40 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->mode = (uns8b)k;
    for ( j = 40, k = 0, m = 1 ; j < 48 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->unc_tipo = (uns8b)k;
    for ( j = 48, k = 0, m = 1 ; j < 56 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->compression_level = (uns8b)k;
    for ( j = 56, k = 0, m = 1 ; j < 88 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->unc_len = k;
    if ( ( raw->mode > 2 ) ||
         ( raw->unc_tipo >= TRP_MAX_T ) ||
         ( raw->compression_level > 9 ) ||
         ( raw->len > raw->unc_len ) ||
         ( ( raw->compression_level == 0 ) && ( raw->len != raw->unc_len ) ) ) {
        trp_gc_free( raw->data );
        trp_gc_free( raw );
        return UNDEF;
    }
    for ( m = 88, p = raw->data ; len ; len--, p++ )
        for ( k = 0, j = 1, *p = 0 ; k < 8 ; k++, m++, j <<= 1 ) {
            i = m;
            do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
            if ( trp_gcry_stego_eject( map, i ) )
                *p |= j;
        }
    res = trp_uncompress( (trp_obj_t *)raw );
    trp_gc_free( raw->data );
    trp_gc_free( raw );
    return res;
}

static trp_obj_t *trp_gcry_stego_extract_old( struct md5_ctx *context, uns8b *map, uns32b cnt_len, uns32b l )
{
    trp_obj_t *res;
    trp_raw_t *raw;
    uns8b *p;
    uns32b msg_len, m, i, j, k;

    cnt_len -= 32;
    for ( j = 0, i = 0, m = 1 ; j < 32 ; j++, m <<= 1 )
        if ( trp_gcry_stego_eject( map, cnt_len + j ) )
            i |= m;
    msg_len = i << 3;
    if ( ( msg_len == 0 ) || ( i & 0xe0000000 ) || ( msg_len + 56 > cnt_len ) )
        return UNDEF;
    if ( ( raw = (trp_raw_t *)trp_raw_internal( i, 0 ) ) == NULL )
        return UNDEF;
    for ( j = 0, k = 0, m = 1 ; j < 8 ; j++, m <<= 1 ) {
        i = msg_len + j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->mode = (uns8b)k;
    for ( j = 8, k = 0, m = 1 ; j < 16 ; j++, m <<= 1 ) {
        i = msg_len + j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->unc_tipo = (uns8b)k;
    for ( j = 16, k = 0, m = 1 ; j < 24 ; j++, m <<= 1 ) {
        i = msg_len + j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->compression_level = (uns8b)k;
    for ( j = 24, k = 0, m = 1 ; j < 56 ; j++, m <<= 1 ) {
        i = msg_len + j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    raw->unc_len = k;
    if ( ( raw->mode > 2 ) ||
         ( raw->unc_tipo >= TRP_MAX_T ) ||
         ( raw->compression_level > 9 ) ||
         ( raw->len > raw->unc_len ) ||
         ( ( raw->compression_level == 0 ) && ( raw->len != raw->unc_len ) ) ) {
        trp_gc_free( raw->data );
        trp_gc_free( raw );
        return UNDEF;
    }
    for ( m = 0, p = raw->data, msg_len >>= 3 ; msg_len ; msg_len--, p++ )
        for ( k = 0, j = 1, *p = 0 ; k < 8 ; k++, m++, j <<= 1 ) {
            i = m;
            do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
            if ( trp_gcry_stego_eject( map, i ) )
                *p |= j;
        }
    res = trp_uncompress( (trp_obj_t *)raw );
    trp_gc_free( raw->data );
    trp_gc_free( raw );
    return res;
}

uns8b trp_gcry_stego_destroy( trp_obj_t *obj, trp_obj_t *pass_phrase )
{
    uns8b *map, *p;
    uns32b cnt_len, len, i, j, k, l, m;
    struct md5_ctx context[ 4 ];

    if ( obj->tipo != TRP_PIX )
        return 1;
    if ( ( map = ((trp_pix_t *)obj)->map.p ) == NULL )
        return 1;
    cnt_len = 3 * ((trp_pix_t *)obj)->w * ((trp_pix_t *)obj)->h;
    if ( cnt_len < 88 )
        return 1;
    for ( l = 1, m = 2 ; m < cnt_len ; l++, m <<= 1 );
    p = trp_csprint( pass_phrase );
    trp_gcry_luby_rackoff_md_initialize( context, p );
    trp_csprint_free( p );
    for ( j = 0, k = 0, m = 1 ; j < 32 ; j++, m <<= 1 ) {
        i = j;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( trp_gcry_stego_eject( map, i ) )
            k |= m;
    }
    len = ( k << 3 ) + 88;
    if ( ( len == 0 ) || ( k & 0xe0000000 ) || ( len > cnt_len ) ) {
        /*
         si prova se è compatibile con il vecchio formato
         */
        cnt_len -= 32;
        for ( j = 0, i = 0, m = 1 ; j < 32 ; j++, m <<= 1 )
            if ( trp_gcry_stego_eject( map, cnt_len + j ) )
                i |= m;
        len = ( i << 3 ) + 56;
        if ( ( len == 0 ) || ( i & 0xe0000000 ) || ( len > cnt_len ) )
            return 1;
        /*
         è compatibile
         */
        pass_phrase = trp_sig64( 0x100000000LL );
        obj = trp_math_random( pass_phrase );
        j = (uns32b)( ((trp_sig64_t *)obj)->val );
        trp_gc_free( obj );
        for ( i = 0 ; i < 32 ; i++, j >>= 1 )
            trp_gcry_stego_inject( map, cnt_len + i, j & 1 );
    } else {
        pass_phrase = trp_sig64( 0x100000000LL );
    }
    for ( m = 0, k = 0 ; m < len ; m++, k--, j >>= 1 ) {
        i = m;
        do i = trp_gcry_luby_rackoff( l, context, i ); while ( i >= cnt_len );
        if ( k == 0 ) {
            obj = trp_math_random( pass_phrase );
            j = (uns32b)( ((trp_sig64_t *)obj)->val );
            trp_gc_free( obj );
            k = 32;
        }
        trp_gcry_stego_inject( map, i, j & 1 );
    }
    trp_gc_free( pass_phrase );
    return 0;
}

#endif

static trp_raw_t *trp_gcry_md_hash_buffer( uns8b encode_only_if_needed, int algo, trp_obj_t *obj )
{
    trp_raw_t *raw = (trp_raw_t *)trp_raw_internal( gcry_md_get_algo_dlen( algo ), 0 ), *r;

    if ( encode_only_if_needed ) {
        if ( obj->tipo == TRP_CORD ) {
            uns8b *p = trp_csprint( obj );
            gcry_md_hash_buffer( algo, raw->data, p, ((trp_cord_t*)obj)->len );
            trp_csprint_free( p );
            return raw;
        }
        if ( obj->tipo == TRP_PIX ) {
            uns8b *p = ((trp_pix_t *)obj)->map.p;
            if ( p ) {
                gcry_md_hash_buffer( algo, raw->data, p, ( ((trp_pix_t *)obj)->w ) * ( ((trp_pix_t *)obj)->h ) );
                return raw;
            }
        }
    }
    if ( obj->tipo == TRP_RAW ) {
        if ( ((trp_raw_t *)obj)->mode )
            if ( ((trp_raw_t *)obj)->compression_level )
                return NULL;
        r = (trp_raw_t *)obj;
    } else {
        uns8b *buf;
        r = (trp_raw_t *)trp_raw_internal( trp_size_internal( obj ), 0 );
        buf = r->data;
        trp_encode_internal( obj, &buf );
    }
    gcry_md_hash_buffer( algo, raw->data, r->data, r->len );
    if ( obj->tipo != TRP_RAW ) {
        trp_gc_free( r->data );
        trp_gc_free( r );
    }
    return raw;
}

static trp_raw_t *trp_gcry_md_hash_path( int algo, trp_obj_t *path )
{
    trp_raw_t *raw;
    uns8b *cpath = trp_csprint( path );
    FILE *fp;
    gcry_md_hd_t hd;
    size_t i;
    uns8b buf[ 32768 ];

    fp = trp_fopen( cpath, "rb" );
    trp_csprint_free( cpath );
    if ( fp == NULL )
        return NULL;
    if ( gcry_md_open( &hd, algo, 0 ) ) {
        fclose( fp );
        return NULL;
    }
    for ( ; ; ) {
        i = fread( buf, 1, 32768, fp );
        if ( i == 0 )
            break;
        gcry_md_write( hd, buf, i );
    }
    fclose( fp );
    raw = (trp_raw_t *)trp_raw_internal( gcry_md_get_algo_dlen( algo ), 0 );
    memcpy( raw->data, gcry_md_read( hd, algo ), raw->len );
    gcry_md_close( hd );
    return raw;
}

static trp_obj_t *trp_gcry_raw2ascii( trp_raw_t *raw )
{
    trp_obj_t *res;
    uns8b *buf, *p, *q;
    uns32b i;

    buf = trp_gc_malloc_atomic( ( raw->len << 1 ) + 1 );
    for ( i = raw->len, p = buf, q = raw->data ; i ; i--, q++ ) {
        *p++ = trp_gcry_hexdigit( *q >> 4 );
        *p++ = trp_gcry_hexdigit( *q & 0xf );
    }
    *p = 0;
    res = trp_cord( buf );
    trp_gc_free( buf );
    return res;
}

static trp_obj_t *trp_gcry_md_hash_basic( uns8b encode_only_if_needed, trp_obj_t *algo, trp_obj_t *obj )
{
    trp_raw_t *raw;
    uns32b aalgo;

    if ( trp_cast_uns32b( algo, &aalgo ) )
        return UNDEF;
    if ( gcry_md_test_algo( aalgo ) )
        return UNDEF;
    if ( ( raw = trp_gcry_md_hash_buffer( encode_only_if_needed, aalgo, obj ) ) == NULL )
        return UNDEF;
    obj = trp_gcry_raw2ascii( raw );
    trp_gc_free( raw->data );
    trp_gc_free( raw );
    return obj;
}

trp_obj_t *trp_gcry_md_hash( trp_obj_t *algo, trp_obj_t *obj )
{
    return trp_gcry_md_hash_basic( 0, algo, obj );
}

trp_obj_t *trp_gcry_md_hash_fast( trp_obj_t *algo, trp_obj_t *obj )
{
    return trp_gcry_md_hash_basic( 1, algo, obj );
}

trp_obj_t *trp_gcry_md_hash_file( trp_obj_t *algo, trp_obj_t *path )
{
    trp_raw_t *raw;
    uns32b aalgo;

    if ( trp_cast_uns32b( algo, &aalgo ) )
        return UNDEF;
    if ( gcry_md_test_algo( aalgo ) )
        return UNDEF;
    if ( ( raw = trp_gcry_md_hash_path( aalgo, path ) ) == NULL )
        return UNDEF;
    path = trp_gcry_raw2ascii( raw );
    trp_gc_free( raw->data );
    trp_gc_free( raw );
    return path;
}


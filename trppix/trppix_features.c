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

#include "./trppix_internal.h"

uns16b trp_pix_colors_type( trp_pix_t *pix, uns16b max_colors )
{
    uns32b n = pix->w * pix->h, i;
    uns16b ss, j;
    trp_pix_color_t *map = pix->map.c, *s;
    uns8b gray = 1, bw = 1;

    for ( i = 0 ; i < n ; i++ ) {
        if ( ( map[ i ].red != map[ i ].green ) ||
             ( map[ i ].green != map[ i ].blue ) ||
             ( map[ i ].alpha != 0xff ) ) {
            gray = 0;
            break;
        }
        if ( ( map[ i ].red != 0 ) && ( map[ i ].red != 0xff ) )
            bw = 0;
    }
    if ( gray )
        return 0xffff - bw;
    if ( max_colors == 0 )
        return 0;
    if ( ( s = malloc( max_colors * sizeof( trp_pix_color_t ) ) ) == NULL )
        return max_colors;
    for ( ss = 0, i = 0 ; i < n ; i++ ) {
        for ( j = 0 ; j < ss ; j++ )
            if ( s[ j ].rgba == map[ i ].rgba )
                break;
        if ( j == ss ) {
            if ( ss == max_colors )
                break;
            s[ ss++ ].rgba = map[ i ].rgba;
        }
    }
    free( s );
    return ss;
}

uns8b trp_pix_has_alpha_low( trp_pix_t *pix )
{
    uns32b n = pix->w * pix->h, i;
    trp_pix_color_t *map = pix->map.c;

    for ( i = 0 ; i < n ; i++ )
        if ( map[ i ].alpha != 0xff )
            return 1;
    return 0;
}

trp_obj_t *trp_pix_has_alpha( trp_obj_t *pix )
{
    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.c == NULL )
        return UNDEF;
    return trp_pix_has_alpha_low( (trp_pix_t *)pix ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_pix_grayp( trp_obj_t *pix )
{
    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.c == NULL )
        return UNDEF;
    return ( ( trp_pix_colors_type( (trp_pix_t *)pix, 0 ) & 0xfffe ) == 0xfffe ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_pix_bwp( trp_obj_t *pix )
{
    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.c == NULL )
        return UNDEF;
    return ( trp_pix_colors_type( (trp_pix_t *)pix, 0 ) == 0xfffe ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_pix_color_count( trp_obj_t *pix, trp_obj_t *color )
{
    uns32b i, n;
    trp_pix_color_t c, *p;

    if ( trp_pix_decode_color_uns8b( color, pix, &( c.red ), &( c.green ),
                                     &( c.blue ), &( c.alpha ) ) )
        return UNDEF;

    for ( p = ((trp_pix_t *)pix)->map.c, i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h, n = 0 ; i ; )
        if ( p[ --i ].rgba == c.rgba )
            ++n;
    return trp_sig64( n );
}

trp_obj_t *trp_pix_get_luminance( trp_obj_t *pix )
{
    trp_pix_color_t *map;
    sig64b tot_r, tot_g, tot_b;
    uns32b n, i;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( map = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    tot_r = tot_g = tot_b = 0;
    n = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h;
    for ( i = 0 ; i < n ; i++, map++ ) {
        tot_r += map->red;
        tot_g += map->green;
        tot_b += map->blue;
    }
    return trp_math_ratio( trp_sig64( tot_r * TRP_PIX_WEIGHT_RED +
                                      tot_g * TRP_PIX_WEIGHT_GREEN +
                                      tot_b * TRP_PIX_WEIGHT_BLUE ),
                           trp_sig64( n ),
                           trp_sig64( 1000 ),
                           NULL );
}

trp_obj_t *trp_pix_get_contrast( trp_obj_t *pix )
{
    trp_obj_t *tn, *ttot1;
    trp_pix_color_t *map;
    sig64b tot1, tot2;
    uns32b n, i, c;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( map = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    tot1 = tot2 = 0;
    n = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h;
    for ( i = 0 ; i < n ; i++, map++ ) {
        c = TRP_PIX_RGB_TO_GRAY_C( map );
        tot1 += c;
        tot2 += c * c;
    }
    tn = trp_sig64( n );
    ttot1 = trp_sig64( tot1 );
    return trp_math_sqrt( trp_math_minus( trp_math_ratio( trp_sig64( tot2 ), tn, NULL ),
                                          trp_math_ratio( trp_math_times( ttot1, ttot1, NULL ), tn, tn, NULL ),
                                          NULL ) );
}

trp_obj_t *trp_pix_gray_histogram( trp_obj_t *pix )
{
    trp_array_t *obj;
    trp_pix_color_t *c;
    uns32b i, h[ 256 ];

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    for ( i = 0 ; i < 256 ; i++ )
        h[ i ] = 0;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        ++h[ TRP_PIX_RGB_TO_GRAY_C( c ) ];
    obj = trp_gc_malloc( sizeof( trp_array_t ) );
    obj->tipo = TRP_ARRAY;
    obj->incr = 1;
    obj->len = 256;
    obj->data = trp_gc_malloc( sizeof( trp_obj_t * ) * 256 );
    for ( i = 0 ; i < 256 ; i++ )
        obj->data[ i ] = trp_sig64( h[ i ] );
    return (trp_obj_t *)obj;
}


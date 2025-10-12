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

typedef struct {
    uns32b w;
    uns32b h;
    flt64b *map;
} trp_pix_lab_t;

static uns8b trp_pix_rgb2lab( trp_pix_t *pix, trp_pix_lab_t *lab,
                              flt64b *Lmean, flt64b *amean, flt64b *bmean,
                              flt64b *Lstdev, flt64b *astdev, flt64b *bstdev )
{
    uns8b *p;
    flt64b *q;
    uns32b w, h, n, i;
    flt64b rat = 1.0 / 3.0, sed = 16.0 / 116.0;
    flt64b r1, g1, b1, r2, g2, b2;
    flt64b Lm, am, bm, Ls, as, bs;

    w = lab->w = pix->w;
    h = lab->h = pix->h;
    n = w * h;
    if ( ( q = malloc( n * 3 * sizeof( flt64b ) ) ) == NULL )
        return 1;
    lab->map = q;
    p = pix->map.p;
    Lm = am = bm = Ls = as = bs = 0.0;
    for ( i = n ; i ; i--, p++ ) {
        r1 = (flt64b)( *p++ ) / 255.0;
        g1 = (flt64b)( *p++ ) / 255.0;
        b1 = (flt64b)( *p++ ) / 255.0;
        r1 = ( r1 > 0.04045 ) ? pow( ( r1 + 0.055 ) / 1.055, 2.4 ) : r1 / 12.92;
        g1 = ( g1 > 0.04045 ) ? pow( ( g1 + 0.055 ) / 1.055, 2.4 ) : g1 / 12.92;
        b1 = ( b1 > 0.04045 ) ? pow( ( b1 + 0.055 ) / 1.055, 2.4 ) : b1 / 12.92;
        r2 = 0.4338906014 * r1 + 0.3762349153 * g1 + 0.1899060464 * b1;
        g2 = 0.2126       * r1 + 0.7152       * g1 + 0.0722       * b1;
        b2 = 0.0177254484 * r1 + 0.1094753083 * g1 + 0.8729553741 * b1;
        r2 = ( r2 > 0.008856 ) ? pow( r2, rat ) : 7.787 * r2 + sed;
        g2 = ( g2 > 0.008856 ) ? pow( g2, rat ) : 7.787 * g2 + sed;
        b2 = ( b2 > 0.008856 ) ? pow( b2, rat ) : 7.787 * b2 + sed;
        *q++ = r1 = 116.0 * g2 - 16.0;
        *q++ = g1 = 500.0 * ( r2 - g2 );
        *q++ = b1 = 200.0 * ( g2 - b2 );
        Lm += r1;
        am += g1;
        bm += b1;
        Ls += r1 * r1;
        as += g1 * g1;
        bs += b1 * b1;
    }
    *Lmean = Lm = Lm / (flt64b)n;
    *amean = am = am / (flt64b)n;
    *bmean = bm = bm / (flt64b)n;
    *Lstdev = sqrt( Ls / (flt64b)n - Lm * Lm );
    *astdev = sqrt( as / (flt64b)n - am * am );
    *bstdev = sqrt( bs / (flt64b)n - bm * bm );
    return 0;
}

static uns8b trp_pix_lab2rgb( trp_pix_lab_t *lab, trp_pix_t *pix )
{
    uns8b *q;
    flt64b *p;
    uns32b w, h, i;
    flt64b rat = 1.0 / 2.4, sed = 16.0 / 116.0, rcu = pow( 0.008856, 1.0 / 3.0 );
    flt64b r1, g1, b1, r2, g2, b2;

    w = lab->w;
    h = lab->h;
    if ( ( pix->w != w ) || ( pix->h != h ) )
        return 1;
    p = lab->map;
    q = pix->map.p;
    for ( i = w * h ; i ; i--, q++ ) {
        r1 = *p++;
        g1 = *p++;
        b1 = *p++;
        g2 = ( r1 + 16.0 ) / 116.0;
        r2 = g1 / 500.0 + g2;
        b2 = g2 - b1 / 200.0;
        r2 = ( r2 > rcu ) ? r2 * r2 * r2 : ( r2 - sed ) / 7.787;
        g2 = ( g2 > rcu ) ? g2 * g2 * g2 : ( g2 - sed ) / 7.787;
        b2 = ( b2 > rcu ) ? b2 * b2 * b2 : ( b2 - sed ) / 7.787;
        r1 =  3.0801172982 * r2 - 1.5372079724 * g2 - 0.542921777  * b2;
        g1 = -0.9209395766 * r2 + 1.8757560609 * g2 + 0.0452055254 * b2;
        b1 =  0.0529507982 * r2 - 0.2040210505 * g2 + 1.1508888918 * b2;
        r1 = ( r1 > 0.0031308 ) ? 1.055 * pow( r1, rat ) - 0.055 : 12.92 * r1;
        g1 = ( g1 > 0.0031308 ) ? 1.055 * pow( g1, rat ) - 0.055 : 12.92 * g1;
        b1 = ( b1 > 0.0031308 ) ? 1.055 * pow( b1, rat ) - 0.055 : 12.92 * b1;
        *q++ = (uns8b)( fmax( 0.0, fmin( 255.0, r1 * 255.0 + 0.5 ) ) );
        *q++ = (uns8b)( fmax( 0.0, fmin( 255.0, g1 * 255.0 + 0.5 ) ) );
        *q++ = (uns8b)( fmax( 0.0, fmin( 255.0, b1 * 255.0 + 0.5 ) ) );
    }
    return 0;
}

static uns8b trp_pix_color_transfer_low( trp_pix_t *pixs, trp_pix_t *pixt )
{
    trp_pix_lab_t slab, tlab;
    flt64b *p;
    flt64b sLm, sam, sbm, sLs, sas, sbs;
    flt64b tLm, tam, tbm, tLs, tas, tbs;
    flt64b Lr, ar, br, Lw, aw, bw;
    uns32b i;
    uns8b res;

    if ( trp_pix_rgb2lab( pixs, &slab, &sLm, &sam, &sbm, &sLs, &sas, &sbs ) )
        return 1;
    if ( trp_pix_rgb2lab( pixt, &tlab, &tLm, &tam, &tbm, &tLs, &tas, &tbs ) ) {
        free( slab.map );
        return 1;
    }
    Lr = tLs / sLs;
    Lw = tLm - sLm * Lr;
    ar = tas / sas;
    aw = tam - sam * ar;
    br = tbs / sbs;
    bw = tbm - sbm * br;
    for ( i = slab.w * slab.h, p = slab.map ; i ; i-- ) {
        *p = *p * Lr + Lw;
        p++;
        *p = *p * ar + aw;
        p++;
        *p = *p * br + bw;
        p++;
    }
    res = trp_pix_lab2rgb( &slab, pixs );
    free( slab.map );
    free( tlab.map );
    return res;
}

trp_obj_t *trp_pix_color_transfer( trp_obj_t *pixs, trp_obj_t *pixt )
{
    trp_obj_t *res;

    if ( trp_pix_is_not_valid( pixs ) || trp_pix_is_not_valid( pixt ) )
        return UNDEF;
    res = trp_pix_clone( pixs );
    if ( res != UNDEF )
        if ( trp_pix_color_transfer_low( (trp_pix_t *)res, (trp_pix_t *)pixt ) ) {
            (void)trp_pix_close( (trp_pix_t *)res );
            res = UNDEF;
        }
    return res;
}

uns8b trp_pix_color_transfer_test( trp_obj_t *pixs, trp_obj_t *pixt )
{
    if ( trp_pix_is_not_valid( pixs ) || trp_pix_is_not_valid( pixt ) )
        return 1;
    return trp_pix_color_transfer_low( (trp_pix_t *)pixs, (trp_pix_t *)pixt );
}

trp_obj_t *trp_pix_lab_distance( trp_obj_t *pix1, trp_obj_t *pix2 )
{
    trp_pix_lab_t lab1, lab2;
    flt64b *p1, *p2;
    flt64b L1, a1, b1, L2, a2, b2;
    flt64b deltaL, deltaa, deltab;
    flt64b tdist;
    uns32b n, i;

    if ( trp_pix_is_not_valid( pix1 ) || trp_pix_is_not_valid( pix2 ) )
        return UNDEF;
    if ( ( ((trp_pix_t *)pix1)->w != ((trp_pix_t *)pix2)->w ) ||
         ( ((trp_pix_t *)pix1)->h != ((trp_pix_t *)pix2)->h ) )
        return UNDEF;
    if ( trp_pix_rgb2lab( (trp_pix_t *)pix1, &lab1, &L1, &a1, &b1, &L2, &a2, &b2 ) )
        return UNDEF;
    if ( trp_pix_rgb2lab( (trp_pix_t *)pix2, &lab2, &L1, &a1, &b1, &L2, &a2, &b2 ) ) {
        free( lab1.map );
        return UNDEF;
    }
    n = lab1.w * lab1.h;
    tdist = 0.0;
    for ( i = n, p1 = lab1.map, p2 = lab2.map ; i ; i-- ) {
        L1 = *p1++;
        a1 = *p1++;
        b1 = *p1++;
        L2 = *p2++;
        a2 = *p2++;
        b2 = *p2++;
        deltaL = L1 - L2;
        deltaa = a1 - a2;
        deltab = b1 - b2;
        /*
         * FIXME
         * sostituire con una distanza più significativa
         */
        tdist += sqrt( deltaL * deltaL + deltaa * deltaa + deltab * deltab );
    }
    free( lab1.map );
    free( lab2.map );
    return trp_double( tdist / (flt64b)n );
}


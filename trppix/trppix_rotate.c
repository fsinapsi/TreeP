/*
    TreeP Run Time Support
    Copyright (C) 2008-2024 Frank Sinapsi

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

#define TRP_PI 3.1415926535897932384626433832795029L
#define TRP_PIX_ROTATE_ANGLE_MIN (0.18L/TRP_PI)

#ifdef  TRP_BIG_ENDIAN
/*! 8 bit access - get */
#define  GET_DATA_BYTE(pdata, n) \
             (*((const uns8b *)(pdata) + (n)))
/*! 8 bit access - set value (0 ... 255) */
#define  SET_DATA_BYTE(pdata, n, val) \
             *((uns8b *)(pdata) + (n)) = (val)
#else  /* TRP_LITTLE_ENDIAN */
/*! 8 bit access - get */
#define  GET_DATA_BYTE(pdata, n) \
             (*(((const uns8b *)(pdata) + ((n)^3))))
/*! 8 bit access - set value (0 ... 255) */
#define  SET_DATA_BYTE(pdata, n, val) \
             *((uns8b *)(pdata) + ((n)^3)) = (val)
#endif  /* TRP_BIG_ENDIAN */
#define COMBINE_PARTIAL(d,s,m) ( ((d) & ~(m)) | ((s) & (m)) )

typedef struct {
    uns32b             w;         /*!< width in pixels                   */
    uns32b             h;         /*!< height in pixels                  */
    uns32b             d;         /*!< depth in bits (bpp)               */
    uns32b             spp;       /*!< number of samples per pixel       */
    uns32b             wpl;       /*!< 32-bit words/line                 */
    uns32b            *data;      /*!< the image data                    */
} PIX;

enum {
    COLOR_RED = 0,        /*!< red color index in RGBA_QUAD    */
    COLOR_GREEN = 1,      /*!< green color index in RGBA_QUAD  */
    COLOR_BLUE = 2,       /*!< blue color index in RGBA_QUAD   */
    L_ALPHA_CHANNEL = 3   /*!< alpha value index in RGBA_QUAD  */
};

static const sig32b  L_RED_SHIFT =
       8 * (sizeof(uns32b) - 1 - COLOR_RED);           /* 24 */
static const sig32b  L_GREEN_SHIFT =
       8 * (sizeof(uns32b) - 1 - COLOR_GREEN);         /* 16 */
static const sig32b  L_BLUE_SHIFT =
       8 * (sizeof(uns32b) - 1 - COLOR_BLUE);          /*  8 */
static const sig32b  L_ALPHA_SHIFT =
       8 * (sizeof(uns32b) - 1 - L_ALPHA_CHANNEL);     /*  0 */

static const sig32b SHIFT_LEFT  = 0;
static const sig32b SHIFT_RIGHT = 1;

static const uns32b lmask32[] = {0x0,
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
    0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
    0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
    0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
    0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff};

static const uns32b rmask32[] = {0x0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};

static void pixDestroy( PIX **ppix )
{
    if ( *ppix ) {
        free( (*ppix)->data );
        free( *ppix );
        *ppix = NULL;
    }
}

static void pixGetDimensions( const PIX *pix, sig32b *pw, sig32b *ph, sig32b *pd )
{
    if (pw) *pw = pix->w;
    if (ph) *ph = pix->h;
    if (pd) *pd = pix->d;
}

static PIX *pixCreateHeader( sig32b width, sig32b height, sig32b depth )
{
    PIX *pixd;

    if ( ( pixd = malloc( sizeof( PIX ) ) ) == NULL )
        return NULL;
    pixd->w = width;
    pixd->h = height;
    pixd->d = depth;
    pixd->wpl = ( width * depth + 31 ) / 32;
    pixd->spp = ( depth == 32 ) ? 3 : 1;
    return pixd;
}

static int pixSetPadBits( PIX *pix, sig32b val )
{
    sig32b i, w, h, d, wpl, endbits, fullwords;
    uns32b mask;
    uns32b *data, *pword;

    pixGetDimensions(pix, &w, &h, &d);
    if ( d == 32 )  /* no padding exists for 32 bpp */
        return 0;

    data = pix->data;
    wpl = pix->wpl;
    endbits = 32 - (((sig64b)w * d) % 32);
    if (endbits == 32)  /* no partial word */
        return 0;
    fullwords = (1LL * w * d) / 32;
    mask = rmask32[endbits];
    if (val == 0)
        mask = ~mask;

    for (i = 0; i < h; i++) {
        pword = data + i * wpl + fullwords;
        if (val == 0) /* clear */
            *pword = *pword & mask;
        else  /* set */
            *pword = *pword | mask;
    }
    return 0;
}

static PIX *pixCreateNoInit( sig32b width, sig32b height, sig32b depth )
{
    PIX *pixd;

    if ( ( pixd = pixCreateHeader( width, height, depth ) ) == NULL )
        return NULL;
    if ( ( pixd->data = malloc( 4LL * pixd->wpl * height ) ) == NULL ) {
        pixDestroy( &pixd );
        return NULL;
    }
    pixSetPadBits( pixd, 0 );
    return pixd;
}

static PIX *pixCreate( sig32b  width, sig32b  height, sig32b  depth )
{
    PIX *pixd;

    if ( ( pixd = pixCreateNoInit( width, height, depth ) ) == NULL )
        return NULL;
    memset( pixd->data, 0, 4LL * pixd->wpl * pixd->h );
    return pixd;
}

static PIX *pixCreateTemplateNoInit( const PIX *pixs )
{
    sig32b w, h, d;
    PIX *pixd;

    pixGetDimensions(pixs, &w, &h, &d);
    if ( ( pixd = pixCreateNoInit( w, h, d ) ) == NULL )
        return NULL;
    pixd->spp = pixs->spp;
    pixSetPadBits( pixd, 0 );
    return pixd;
}

static PIX *pixCreateTemplate( const PIX  *pixs )
{
    PIX *pixd;

    if ( ( pixd = pixCreateTemplateNoInit( pixs ) ) == NULL )
        return NULL;
    memset( pixd->data, 0, 4LL * pixd->wpl * pixd->h );
    return pixd;
}

static void
rasteropWordAlignedLow(uns32b  *datad,
                       sig32b    dwpl,
                       sig32b    dx,
                       sig32b    dy,
                       sig32b    dw,
                       sig32b    dh,
                       uns32b  *datas,
                       sig32b    swpl,
                       sig32b    sx,
                       sig32b    sy)
{
    sig32b    nfullw;     /* number of full words */
    uns32b  *psfword;    /* ptr to first src word */
    uns32b  *pdfword;    /* ptr to first dest word */
    sig32b    lwbits;     /* number of ovrhang bits in last partial word */
    uns32b   lwmask;     /* mask for last partial word */
    uns32b  *lines, *lined;
    sig32b    i, j;

    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
    nfullw = dw >> 5;
    lwbits = dw & 31;
    if (lwbits)
        lwmask = lmask32[lwbits];
    psfword = datas + swpl * sy + (sx >> 5);
    pdfword = datad + dwpl * dy + (dx >> 5);

    for (i = 0; i < dh; i++) {
        lines = psfword + i * swpl;
        lined = pdfword + i * dwpl;
        for (j = 0; j < nfullw; j++) {
            *lined = *lines;
            lined++;
            lines++;
        }
        if (lwbits)
            *lined = COMBINE_PARTIAL(*lined, *lines, lwmask);
    }
}

static void
rasteropVAlignedLow(uns32b  *datad,
                    sig32b    dwpl,
                    sig32b    dx,
                    sig32b    dy,
                    sig32b    dw,
                    sig32b    dh,
                    uns32b  *datas,
                    sig32b    swpl,
                    sig32b    sx,
                    sig32b    sy)
{
    sig32b    dfwpartb;   /* boolean (1, 0) if first dest word is partial */
    sig32b    dfwpart2b;  /* boolean (1, 0) if first dest word is doubly partial */
    uns32b   dfwmask;    /* mask for first partial dest word */
    sig32b    dfwbits;    /* first word dest bits in ovrhang */
    uns32b  *pdfwpart;   /* ptr to first partial dest word */
    uns32b  *psfwpart;   /* ptr to first partial src word */
    sig32b    dfwfullb;   /* boolean (1, 0) if there exists a full dest word */
    sig32b    dnfullw;    /* number of full words in dest */
    uns32b  *pdfwfull;   /* ptr to first full dest word */
    uns32b  *psfwfull;   /* ptr to first full src word */
    sig32b    dlwpartb;   /* boolean (1, 0) if last dest word is partial */
    uns32b   dlwmask;    /* mask for last partial dest word */
    sig32b    dlwbits;    /* last word dest bits in ovrhang */
    uns32b  *pdlwpart;   /* ptr to last partial dest word */
    uns32b  *pslwpart;   /* ptr to last partial src word */
    sig32b    i, j;

    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
        /* is the first word partial? */
    dfwmask = 0;
    if ((dx & 31) == 0) {  /* if not */
        dfwpartb = 0;
        dfwbits = 0;
    } else {  /* if so */
        dfwpartb = 1;
        dfwbits = 32 - (dx & 31);
        dfwmask = rmask32[dfwbits];
        pdfwpart = datad + dwpl * dy + (dx >> 5);
        psfwpart = datas + swpl * sy + (sx >> 5);
    }

        /* is the first word doubly partial? */
    if (dw >= dfwbits) {  /* if not */
        dfwpart2b = 0;
    } else {  /* if so */
        dfwpart2b = 1;
        dfwmask &= lmask32[32 - dfwbits + dw];
    }

        /* is there a full dest word? */
    if (dfwpart2b == 1) {  /* not */
        dfwfullb = 0;
        dnfullw = 0;
    } else {
        dnfullw = (dw - dfwbits) >> 5;
        if (dnfullw == 0) {  /* if not */
            dfwfullb = 0;
        } else {  /* if so */
            dfwfullb = 1;
            if (dfwpartb) {
                pdfwfull = pdfwpart + 1;
                psfwfull = psfwpart + 1;
            } else {
                pdfwfull = datad + dwpl * dy + (dx >> 5);
                psfwfull = datas + swpl * sy + (sx >> 5);
            }
        }
    }

        /* is the last word partial? */
    dlwbits = (dx + dw) & 31;
    if (dfwpart2b == 1 || dlwbits == 0) {  /* if not */
        dlwpartb = 0;
    } else {
        dlwpartb = 1;
        dlwmask = lmask32[dlwbits];
        if (dfwpartb) {
            pdlwpart = pdfwpart + 1 + dnfullw;
            pslwpart = psfwpart + 1 + dnfullw;
        } else {
            pdlwpart = datad + dwpl * dy + (dx >> 5) + dnfullw;
            pslwpart = datas + swpl * sy + (sx >> 5) + dnfullw;
        }
    }

    if (dfwpartb) {
        for (i = 0; i < dh; i++) {
            *pdfwpart = COMBINE_PARTIAL(*pdfwpart, *psfwpart, dfwmask);
            pdfwpart += dwpl;
            psfwpart += swpl;
        }
    }

    /* do the full words */
    if (dfwfullb) {
        for (i = 0; i < dh; i++) {
            for (j = 0; j < dnfullw; j++)
                *(pdfwfull + j) = *(psfwfull + j);
            pdfwfull += dwpl;
            psfwfull += swpl;
        }
    }

    /* do the last partial word */
    if (dlwpartb) {
        for (i = 0; i < dh; i++) {
            *pdlwpart = COMBINE_PARTIAL(*pdlwpart, *pslwpart, dlwmask);
            pdlwpart += dwpl;
            pslwpart += swpl;
        }
    }
}

static void
rasteropGeneralLow(uns32b  *datad,
                   sig32b    dwpl,
                   sig32b    dx,
                   sig32b    dy,
                   sig32b    dw,
                   sig32b    dh,
                   uns32b  *datas,
                   sig32b    swpl,
                   sig32b    sx,
                   sig32b    sy)
{
    sig32b    dfwpartb;    /* boolean (1, 0) if first dest word is partial      */
    sig32b    dfwpart2b;   /* boolean (1, 0) if 1st dest word is doubly partial */
    uns32b   dfwmask;     /* mask for first partial dest word                  */
    sig32b    dfwbits;     /* first word dest bits in overhang; 0-31            */
    sig32b    dhang;       /* dest overhang in first partial word,              */
    /* or 0 if dest is word aligned (same as dfwbits)    */
    uns32b  *pdfwpart;    /* ptr to first partial dest word                    */
    uns32b  *psfwpart;    /* ptr to first partial src word                     */
    sig32b    dfwfullb;    /* boolean (1, 0) if there exists a full dest word   */
    sig32b    dnfullw;     /* number of full words in dest                      */
    uns32b  *pdfwfull;    /* ptr to first full dest word                       */
    uns32b  *psfwfull;    /* ptr to first full src word                        */
    sig32b    dlwpartb;    /* boolean (1, 0) if last dest word is partial       */
    uns32b   dlwmask;     /* mask for last partial dest word                   */
    sig32b    dlwbits;     /* last word dest bits in ovrhang                    */
    uns32b  *pdlwpart;    /* ptr to last partial dest word                     */
    uns32b  *pslwpart;    /* ptr to last partial src word                      */
    uns32b   sword;       /* compose src word aligned with the dest words      */
    sig32b    sfwbits;     /* first word src bits in overhang (1-32),           */
    /* or 32 if src is word aligned                      */
    sig32b    shang;       /* source overhang in the first partial word,        */
    /* or 0 if src is word aligned (not same as sfwbits) */
    sig32b    sleftshift;  /* bits to shift left for source word to align       */
    /* with the dest.  Also the number of bits that      */
    /* get shifted to the right to align with the dest.  */
    sig32b    srightshift; /* bits to shift right for source word to align      */
    /* with dest.  Also, the number of bits that get     */
    /* shifted left to align with the dest.              */
    sig32b    srightmask;  /* mask for selecting sleftshift bits that have      */
    /* been shifted right by srightshift bits            */
    sig32b    sfwshiftdir; /* either SHIFT_LEFT or SHIFT_RIGHT                  */
    sig32b    sfwaddb;     /* boolean: do we need an additional sfw right shift? */
    sig32b    slwaddb;     /* boolean: do we need an additional slw right shift? */
    sig32b    i, j;

    /*--------------------------------------------------------*
     *                Preliminary calculations                *
     *--------------------------------------------------------*/
        /* To get alignment of src with dst (e.g., in the
         * full words) the src must do a left shift of its
         * relative overhang in the current src word,
         * and OR that with a right shift of
         * (31 -  relative overhang) from the next src word.
         * We find the absolute overhangs, the relative overhangs,
         * the required shifts and the src mask */
    if ((sx & 31) == 0)
        shang = 0;
    else
        shang = 32 - (sx & 31);
    if ((dx & 31) == 0)
        dhang = 0;
    else
        dhang = 32 - (dx & 31);

    if (shang == 0 && dhang == 0) {  /* this should be treated by an
                                        aligned operation, not by
                                        this general rasterop! */
        sleftshift = 0;
        srightshift = 0;
        srightmask = rmask32[0];
    } else {
        if (dhang > shang)
            sleftshift = dhang - shang;
        else
            sleftshift = 32 - (shang - dhang);
        srightshift = 32 - sleftshift;
        srightmask = rmask32[sleftshift];
    }

        /* Is the first dest word partial? */
    dfwmask = 0;
    if ((dx & 31) == 0) {  /* if not */
        dfwpartb = 0;
        dfwbits = 0;
    } else {  /* if so */
        dfwpartb = 1;
        dfwbits = 32 - (dx & 31);
        dfwmask = rmask32[dfwbits];
        pdfwpart = datad + dwpl * dy + (dx >> 5);
        psfwpart = datas + swpl * sy + (sx >> 5);
        sfwbits = 32 - (sx & 31);
        if (dfwbits > sfwbits) {
            sfwshiftdir = SHIFT_LEFT;  /* shift by sleftshift */
                /* Do we have enough bits from the current src word? */
            if (dw <= shang)
                sfwaddb = 0;  /* yes: we have enough bits */
            else
                sfwaddb = 1;  /* no: rshift in next src word by srightshift */
        } else {
            sfwshiftdir = SHIFT_RIGHT;  /* shift by srightshift */
        }
    }

        /* Is the first dest word doubly partial? */
    if (dw >= dfwbits) {  /* if not */
        dfwpart2b = 0;
    } else {  /* if so */
        dfwpart2b = 1;
        dfwmask &= lmask32[32 - dfwbits + dw];
    }

        /* Is there a full dest word? */
    if (dfwpart2b == 1) {  /* not */
        dfwfullb = 0;
        dnfullw = 0;
    } else {
        dnfullw = (dw - dfwbits) >> 5;
        if (dnfullw == 0) {  /* if not */
            dfwfullb = 0;
        } else {  /* if so */
            dfwfullb = 1;
            pdfwfull = datad + dwpl * dy + ((dx + dhang) >> 5);
            psfwfull = datas + swpl * sy + ((sx + dhang) >> 5); /* yes, dhang */
        }
    }

        /* Is the last dest word partial? */
    dlwbits = (dx + dw) & 31;
    if (dfwpart2b == 1 || dlwbits == 0) {  /* if not */
        dlwpartb = 0;
    } else {
        dlwpartb = 1;
        dlwmask = lmask32[dlwbits];
        pdlwpart = datad + dwpl * dy + ((dx + dhang) >> 5) + dnfullw;
        pslwpart = datas + swpl * sy + ((sx + dhang) >> 5) + dnfullw;
        if (dlwbits <= srightshift)   /* must be <= here !!! */
            slwaddb = 0;  /* we got enough bits from current src word */
        else
            slwaddb = 1;  /* must rshift in next src word by srightshift */
    }

    if (dfwpartb) {
        for (i = 0; i < dh; i++)
        {
            if (sfwshiftdir == SHIFT_LEFT) {
                sword = *psfwpart << sleftshift;
                if (sfwaddb)
                    sword = COMBINE_PARTIAL(sword,
                                            *(psfwpart + 1) >> srightshift,
                                            srightmask);
            } else {  /* shift right */
                sword = *psfwpart >> srightshift;
            }

            *pdfwpart = COMBINE_PARTIAL(*pdfwpart, sword, dfwmask);
            pdfwpart += dwpl;
            psfwpart += swpl;
        }
    }

    /* do the full words */
    if (dfwfullb) {
        for (i = 0; i < dh; i++) {
            for (j = 0; j < dnfullw; j++) {
                sword = COMBINE_PARTIAL(*(psfwfull + j) << sleftshift,
                                        *(psfwfull + j + 1) >> srightshift,
                                        srightmask);
                *(pdfwfull + j) = sword;
            }
            pdfwfull += dwpl;
            psfwfull += swpl;
        }
    }

    /* do the last partial word */
    if (dlwpartb) {
        for (i = 0; i < dh; i++) {
            sword = *pslwpart << sleftshift;
            if (slwaddb)
                sword = COMBINE_PARTIAL(sword,
                                        *(pslwpart + 1) >> srightshift,
                                        srightmask);

                *pdlwpart = COMBINE_PARTIAL(*pdlwpart, sword, dlwmask);
            pdlwpart += dwpl;
            pslwpart += swpl;
        }
    }
}

static void
rasteropLow(uns32b  *datad,
            sig32b    dpixw,
            sig32b    dpixh,
            sig32b    depth,
            sig32b    dwpl,
            sig32b    dx,
            sig32b    dy,
            sig32b    dw,
            sig32b    dh,
            uns32b  *datas,
            sig32b    spixw,
            sig32b    spixh,
            sig32b    swpl,
            sig32b    sx,
            sig32b    sy)
{
sig32b  dhangw, shangw, dhangh, shangh;

   /* -------------------------------------------------------*
    *            Scale horizontal dimensions by depth        *
    * -------------------------------------------------------*/
    if (depth != 1) {
        dpixw *= depth;
        dx *= depth;
        dw *= depth;
        spixw *= depth;
        sx *= depth;
    }

   /* -------------------------------------------------------*
    *      Clip to max rectangle within both src and dest    *
    * -------------------------------------------------------*/
       /* Clip horizontally (sx, dx, dw) */
    if (dx < 0) {
        sx -= dx;  /* increase sx */
        dw += dx;  /* reduce dw */
        dx = 0;
    }
    if (sx < 0) {
        dx -= sx;  /* increase dx */
        dw += sx;  /* reduce dw */
        sx = 0;
    }
    dhangw = dx + dw - dpixw;  /* rect ovhang dest to right */
    if (dhangw > 0)
        dw -= dhangw;  /* reduce dw */
    shangw = sx + dw - spixw;   /* rect ovhang src to right */
    if (shangw > 0)
        dw -= shangw;  /* reduce dw */

       /* Clip vertically (sy, dy, dh) */
    if (dy < 0) {
        sy -= dy;  /* increase sy */
        dh += dy;  /* reduce dh */
        dy = 0;
    }
    if (sy < 0) {
        dy -= sy;  /* increase dy */
        dh += sy;  /* reduce dh */
        sy = 0;
    }
    dhangh = dy + dh - dpixh;  /* rect ovhang dest below */
    if (dhangh > 0)
        dh -= dhangh;  /* reduce dh */
    shangh = sy + dh - spixh;  /* rect ovhang src below */
    if (shangh > 0)
        dh -= shangh;  /* reduce dh */

        /* If clipped entirely, quit */
    if ((dw <= 0) || (dh <= 0))
        return;

   /* -------------------------------------------------------*
    *       Dispatch to aligned or non-aligned blitters      *
    * -------------------------------------------------------*/
    if (((dx & 31) == 0) && ((sx & 31) == 0))
        rasteropWordAlignedLow(datad, dwpl, dx, dy, dw, dh,
                               datas, swpl, sx, sy);
    else if ((dx & 31) == (sx & 31))
        rasteropVAlignedLow(datad, dwpl, dx, dy, dw, dh,
                            datas, swpl, sx, sy);
    else
        rasteropGeneralLow(datad, dwpl, dx, dy, dw, dh,
                           datas, swpl, sx, sy);
}

static int
pixRasterop(PIX     *pixd,
            sig32b  dx,
            sig32b  dy,
            sig32b  dw,
            sig32b  dh,
            PIX     *pixs,
            sig32b  sx,
            sig32b  sy)
{
    sig32b  dpw, dph, dpd, spw, sph, spd;

    pixGetDimensions(pixd, &dpw, &dph, &dpd);

        /* Two-image rasterop; the depths must match */
    pixGetDimensions(pixs, &spw, &sph, &spd);
    if (dpd != spd)
        return 1;

    rasteropLow(pixd->data, dpw, dph, dpd, pixd->wpl,
                dx, dy, dw, dh,
                pixs->data, spw, sph, pixs->wpl, sx, sy);
    return 0;
}

static PIX *pixEmbedForRotation( PIX *pixs, flt64b angle )
{
    sig32b w, h, d, w1, h1, w2, h2, wnew, hnew, xoff, yoff;
    flt64b sina, cosa, fw, fh;
    PIX *pixd;

    pixGetDimensions( pixs, &w, &h, &d );

        /* Find the new sizes required to hold the image after rotation.
         * Note that the new dimensions must be at least as large as those
         * of pixs, because we're rasterop-ing into it before rotation. */
    sina = sin( angle );
    cosa = cos( angle );
    fw = (flt64b)w;
    fh = (flt64b)h;
    w1 = (sig32b)( TRP_ABS( fw * cosa - fh * sina ) + 0.5 );
    w2 = (sig32b)( TRP_ABS( -fw * cosa - fh * sina ) + 0.5 );
    h1 = (sig32b)( TRP_ABS( fw * sina + fh * cosa ) + 0.5 );
    h2 = (sig32b)( TRP_ABS( -fw * sina + fh * cosa ) + 0.5 );
    wnew = TRP_MAX( w, TRP_MAX( w1, w2 ) );
    hnew = TRP_MAX( h, TRP_MAX( h1, h2 ) );

    if ( ( pixd = pixCreateNoInit( wnew, hnew, d ) ) == NULL )
        return NULL;
    pixd->spp = pixs->spp;
    xoff = ( wnew - w ) / 2;
    yoff = ( hnew - h ) / 2;

        /* Set background to color to be rotated in */
    memset( pixd->data, 0xff, 4LL * pixd->wpl * pixd->h );

        /* Rasterop automatically handles all 4 channels for rgba */
    pixRasterop( pixd, xoff, yoff, w, h, pixs, 0, 0 );
    return pixd;
}

static PIX *pixGetRGBComponent( PIX *pixs, sig32b comp )
{
    sig32b i, j, w, h, wpls, wpld;
    uns32b *lines, *lined;
    PIX *pixd;

    pixGetDimensions( pixs, &w, &h, NULL );
    if ( ( pixd = pixCreate( w, h, 8 ) ) == NULL )
        return NULL;
    wpls = pixs->wpl;
    wpld = pixd->wpl;
    lines = pixs->data;
    lined = pixd->data;
    for ( i = 0 ; i < h ; i++, lines += wpls, lined += wpld )
        for ( j = 0 ; j < w ; j++ )
            SET_DATA_BYTE( lined, j, GET_DATA_BYTE( lines + j, comp ) );
    return pixd;
}

static void pixSetRGBComponent( PIX *pixd, PIX *pixs, sig32b comp )
{
    uns8b srcbyte;
    sig32b i, j, w, h;
    sig32b wpls, wpld;
    uns32b *lines, *lined;

    pixGetDimensions( pixs, &w, &h, NULL);
    if ( comp == L_ALPHA_CHANNEL )
        pixd->spp = 4;
    wpls = pixs->wpl;
    wpld = pixd->wpl;
    lines = pixs->data;
    lined = pixd->data;
    for ( i = 0; i < h; i++, lines += wpls, lined += wpld )
        for ( j = 0 ; j < w ; j++ )
            SET_DATA_BYTE( lined + j, comp, GET_DATA_BYTE( lines, j ) );
}

static void composeRGBPixel( sig32b rval, sig32b gval, sig32b bval, uns32b *ppixel )
{
    *ppixel = ((uns32b)rval << L_RED_SHIFT) |
              ((uns32b)gval << L_GREEN_SHIFT) |
              ((uns32b)bval << L_BLUE_SHIFT);
}

static void rotateAMColorLow( uns32b *datad, sig32b w, sig32b h, sig32b wpld, uns32b *datas, sig32b wpls, flt64b angle )
{
    sig32b i, j, xcen, ycen, wm2, hm2;
    sig32b xdif, ydif, xpm, ypm, xp, yp, xf, yf;
    sig32b rval, gval, bval;
    uns32b word00, word01, word10, word11;
    uns32b *lines, *lined;
    flt64b sina, cosa;

    xcen = w / 2;
    wm2 = w - 2;
    ycen = h / 2;
    hm2 = h - 2;
    sina = 16.0 * sin( angle );
    cosa = 16.0 * cos( angle );

    for (i = 0; i < h; i++) {
        ydif = ycen - i;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            xdif = xcen - j;
            xpm = (sig32b)(-xdif * cosa - ydif * sina);
            ypm = (sig32b)(-ydif * cosa + xdif * sina);
            xp = xcen + (xpm >> 4);
            yp = ycen + (ypm >> 4);
            xf = xpm & 0x0f;
            yf = ypm & 0x0f;

                /* if off the edge, write input colorval */
            if (xp < 0 || yp < 0 || xp > wm2 || yp > hm2) {
                *(lined + j) = 0xffffff00;
                continue;
            }

            lines = datas + yp * wpls;

                /* do area weighting.  Without this, we would
                 * simply do:
                 *   *(lined + j) = *(lines + xp);
                 * which is faster but gives lousy results!
                 */
            word00 = *(lines + xp);
            word10 = *(lines + xp + 1);
            word01 = *(lines + wpls + xp);
            word11 = *(lines + wpls + xp + 1);
            rval = ((16 - xf) * (16 - yf) * ((word00 >> L_RED_SHIFT) & 0xff) +
                    xf * (16 - yf) * ((word10 >> L_RED_SHIFT) & 0xff) +
                    (16 - xf) * yf * ((word01 >> L_RED_SHIFT) & 0xff) +
                    xf * yf * ((word11 >> L_RED_SHIFT) & 0xff) + 128) / 256;
            gval = ((16 - xf) * (16 - yf) * ((word00 >> L_GREEN_SHIFT) & 0xff) +
                    xf * (16 - yf) * ((word10 >> L_GREEN_SHIFT) & 0xff) +
                    (16 - xf) * yf * ((word01 >> L_GREEN_SHIFT) & 0xff) +
                    xf * yf * ((word11 >> L_GREEN_SHIFT) & 0xff) + 128) / 256;
            bval = ((16 - xf) * (16 - yf) * ((word00 >> L_BLUE_SHIFT) & 0xff) +
                    xf * (16 - yf) * ((word10 >> L_BLUE_SHIFT) & 0xff) +
                    (16 - xf) * yf * ((word01 >> L_BLUE_SHIFT) & 0xff) +
                    xf * yf * ((word11 >> L_BLUE_SHIFT) & 0xff) + 128) / 256;
            composeRGBPixel(rval, gval, bval, lined + j);
        }
    }
}

static void rotateAMGrayLow( uns32b *datad, sig32b w, sig32b h, sig32b wpld, uns32b *datas, sig32b wpls, flt64b angle )
{
    sig32b i, j, xcen, ycen, wm2, hm2;
    sig32b xdif, ydif, xpm, ypm, xp, yp, xf, yf;
    sig32b v00, v01, v10, v11;
    uns8b val;
    uns32b *lines, *lined;
    flt64b sina, cosa;

    xcen = w / 2;
    wm2 = w - 2;
    ycen = h / 2;
    hm2 = h - 2;
    sina = 16.0 * sin( angle );
    cosa = 16.0 * cos( angle );

    for (i = 0; i < h; i++) {
        ydif = ycen - i;
        lined = datad + i * wpld;
        for (j = 0; j < w; j++) {
            xdif = xcen - j;
            xpm = (sig32b)(-xdif * cosa - ydif * sina);
            ypm = (sig32b)(-ydif * cosa + xdif * sina);
            xp = xcen + (xpm >> 4);
            yp = ycen + (ypm >> 4);
            xf = xpm & 0x0f;
            yf = ypm & 0x0f;

                /* if off the edge, write input grayval */
            if (xp < 0 || yp < 0 || xp > wm2 || yp > hm2) {
                SET_DATA_BYTE( lined, j, 255 );
                continue;
            }

            lines = datas + yp * wpls;

                /* do area weighting.  Without this, we would
                 * simply do:
                 *   SET_DATA_BYTE(lined, j, GET_DATA_BYTE(lines, xp));
                 * which is faster but gives lousy results!
                 */
            v00 = (16 - xf) * (16 - yf) * GET_DATA_BYTE(lines, xp);
            v10 = xf * (16 - yf) * GET_DATA_BYTE(lines, xp + 1);
            v01 = (16 - xf) * yf * GET_DATA_BYTE(lines + wpls, xp);
            v11 = xf * yf * GET_DATA_BYTE(lines + wpls, xp + 1);
            val = (uns8b)((v00 + v01 + v10 + v11 + 128) / 256);
            SET_DATA_BYTE(lined, j, val);
        }
    }
}

/*******************************************************************************************
 *
 *
 * da qui in poi ho scritto io
 *
 *
 *******************************************************************************************/

static uns8b trp_pix_rotate_test_low( uns32b *w, uns32b *h, trp_pix_color_t *c, sig64b a )
{
    uns32b i, j, n, m;

    switch ( ( a >= 0 ) ? ( a % 360 ) : ( 360 - ( (-a) % 360 ) ) ) {
    case 0:
        return 0;
    case 90:
        {
            trp_pix_color_t *d;

            n = ( *w * *h ) << 2;
            if ( ( d = malloc( n ) ) == NULL )
                return 1;
            j = *w * *h - 1;
            for ( i = 0 ; i <= j ; i++ )
                d[ ( *w - 1 - i / *h ) * *h + ( i % *h ) ].rgba = c[ ( i == j ) ? j : ( ( (uns64b)i * (uns64b)(*w) ) % (uns64b)j ) ].rgba;
            memcpy( c, d, n );
            free( d );
        }
        i = *w;
        *w = *h;
        *h = i;
        return 0;
    case 180:
        for ( n = 0, i = 0 ; i < ( ( *h + 1 ) >> 1 ) ; i++ )
            for ( j = 0 ; j < *w ; j++, n++ ) {
                m = ( *h - 1 - i ) * *w + ( *w - 1 - j );
                if ( n < m ) {
                    a = c[ n ].rgba;
                    c[ n ].rgba = c[ m ].rgba;
                    c[ m ].rgba = a;
                }
            }
        return 0;
    case 270:
        {
            trp_pix_color_t *d;

            n = ( *w * *h ) << 2;
            if ( ( d = malloc( n ) ) == NULL )
                return 1;
            j = *w * *h - 1;
            for ( i = 0 ; i <= j ; i++ )
                d[ i - ( ( i % *h ) << 1 ) + *h - 1 ].rgba = c[ ( i == j ) ? j : ( ( (uns64b)i * (uns64b)(*w) ) % (uns64b)j ) ].rgba;
            memcpy( c, d, n );
            free( d );
        }
        i = *w;
        *w = *h;
        *h = i;
        return 0;
    }
    return 1;
}

uns8b trp_pix_rotate_test( trp_obj_t *pix, trp_obj_t *angle )
{
    trp_pix_color_t *c;
    sig64b a;

    if ( ( ( c = trp_pix_get_mapc( pix ) ) == NULL ) || trp_cast_sig64b( angle, &a ) )
        return 1;
    return trp_pix_rotate_test_low( &(((trp_pix_t *)pix)->w), &(((trp_pix_t *)pix)->h), c, a );
}

uns8b trp_pix_rotate_low( trp_obj_t *pix, flt64b a, uns32b *wo, uns32b *ho, uns8b **data )
{
    trp_pix_color_t *c;
    sig64b aint = -1;
    uns32b w, h;

    if ( ( c = trp_pix_get_mapc( pix ) ) == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    while ( a >= 360.0 )
        a -= 360.0;
    while ( a < 0.0 )
        a += 360.0;
    if ( a < TRP_PIX_ROTATE_ANGLE_MIN )
        aint = 0;
    else if ( TRP_ABS( a - 90.0 ) < TRP_PIX_ROTATE_ANGLE_MIN )
        aint = 90;
    else if ( TRP_ABS( a - 180.0 ) < TRP_PIX_ROTATE_ANGLE_MIN )
        aint = 180;
    else if ( TRP_ABS( a - 270.0 ) < TRP_PIX_ROTATE_ANGLE_MIN )
        aint = 270;
    if ( aint >= 0 ) {
        if ( ( *data = malloc( ( w * h ) << 2 ) ) == NULL )
            return 1;
        memcpy( *data, c, ( w * h ) << 2 );
        trp_pix_rotate_test_low( &w, &h, (trp_pix_color_t *)( *data ), aint );
        *wo = w;
        *ho = h;
    } else {
        PIX *pix, *pixt1, *pixt2;
        trp_pix_color_t *d;
        uns32b i;

        a = ( 360.0 - a ) * (TRP_PI/180.0);
        /*
         * si = fabs( sin( a ) );
         * co = fabs( cos( a ) );
         * ww = (flt64b)w * co + (flt64b)h * si + 0.5;
         * hh = (flt64b)h * co + (flt64b)w * si + 0.5;
         */
        if ( ( pix = pixCreateNoInit( w, h, 32 ) ) == NULL )
            return 1;
        d = (trp_pix_color_t *)( pix->data );
        for ( i = w * h ; i ; ) {
            i--;
            d[ i ].alpha = c[ i ].red;
            d[ i ].blue = c[ i ].green;
            d[ i ].green = c[ i ].blue;
            d[ i ].red = ~( c[ i ].alpha );
        }
        pix->spp = 4;
        pixt1 = pixEmbedForRotation( pix, a );
        pixDestroy( &pix );
        if ( pixt1 == NULL )
            return 1;
        pix = pixCreateTemplate( pixt1 );
        if ( pix == NULL ) {
            pixDestroy( &pixt1 );
            return 1;
        }
        rotateAMColorLow( pix->data, pix->w, pix->h, pix->wpl, pixt1->data, pixt1->wpl, a );
        pixt2 = pixGetRGBComponent( pixt1, L_ALPHA_CHANNEL );
        pixDestroy( &pixt1 );
        if ( pixt2 == NULL ) {
            pixDestroy( &pix );
            return 1;
        }
        pixt1 = pixCreateTemplate( pixt2 );
        if ( pixt1 == NULL ) {
            pixDestroy( &pixt2 );
            pixDestroy( &pix );
            return 1;
        }
        rotateAMGrayLow( pixt1->data, pixt1->w, pixt1->h, pixt1->wpl, pixt2->data, pixt2->wpl, a );
        pixDestroy( &pixt2 );
        pixSetRGBComponent( pix, pixt1, L_ALPHA_CHANNEL );
        pixDestroy( &pixt1 );
        *wo = pix->w;
        *ho = pix->h;
        if ( ( *data = malloc( ( *wo * *ho ) << 2 ) ) == NULL ) {
            pixDestroy( &pix );
            return 1;
        }
        c = (trp_pix_color_t *)( *data );
        d = (trp_pix_color_t *)( pix->data );
        for ( i = *wo * *ho ; i ; ) {
            i--;
            c[ i ].red = d[ i ].alpha;
            c[ i ].green = d[ i ].blue;
            c[ i ].blue = d[ i ].green;
            c[ i ].alpha = ~( d[ i ].red );
        }
        pixDestroy( &pix );
    }
    return 0;
}

trp_obj_t *trp_pix_rotate( trp_obj_t *pix, trp_obj_t *angle )
{
    uns8b *data;
    uns32b w, h;
    flt64b a;

    if ( trp_cast_flt64b( angle, &a ) )
        return UNDEF;
    if ( trp_pix_rotate_low( pix, a, &w, &h, &data ) )
        return UNDEF;
    return trp_pix_create_image_from_data( 0, w, h, data );
}


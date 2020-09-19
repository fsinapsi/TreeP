/*
 * bitstream.c
 * Copyright (C) 2004 Gildas Bazin <gbazin@videolan.org>
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of libdca, a free DTS Coherent Acoustics stream decoder.
 * See http://www.videolan.org/developers/libdca.html for updates.
 *
 * libdca is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdca is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "../trp/trp.h"
#include "dca.h"
#include "dca_internal.h"
#include "dca_bitstream.h"

#define BUFFER_SIZE 4096

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795029
#endif

int dca_sample_rates[] = {
    0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0,
    12000, 24000, 48000, 96000, 192000
};

int dca_bit_rates[] = {
    32000, 56000, 64000, 96000, 112000, 128000,
    192000, 224000, 256000, 320000, 384000,
    448000, 512000, 576000, 640000, 768000,
    896000, 1024000, 1152000, 1280000, 1344000,
    1408000, 1411200, 1472000, 1536000, 1920000,
    2048000, 3072000, 3840000, 1/*open*/, 2/*variable*/, 3/*lossless*/
};

uns8b dca_channels[] = {
    1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8
};

uns8b dca_bits_per_sample[] = {
    16, 16, 20, 20, 0, 24, 24
};

void dca_bitstream_init (dca_state_t * state, uns8b * buf, int word_mode,
                         int bigendian_mode)
{
#if __WORDSIZE == 64
    uns64b align = ((uns64b)buf) & 3;
    state->buffer_start = (uns32b *) (((uns64b)buf) - align);
#else
    uns32b align = ((uns32b)buf) & 3;
    state->buffer_start = (uns32b *) (((uns32b)buf) - align);
#endif
    state->bits_left = 0;
    state->current_word = 0;
    state->word_mode = word_mode;
    state->bigendian_mode = bigendian_mode;
    bitstream_get (state, align << 3);
}
#include<stdio.h>
static inline void bitstream_fill_current (dca_state_t * state)
{
    uns32b tmp;

    tmp = *(state->buffer_start++);

    if (state->bigendian_mode)
        state->current_word = swab32 (tmp);
    else
        state->current_word = swable32 (tmp);

    if (!state->word_mode)
    {
        state->current_word = (state->current_word & 0x00003FFF) |
            ((state->current_word & 0x3FFF0000 ) >> 2);
    }
}

/*
 * The fast paths for _get is in the
 * bitstream.h header file so it can be inlined.
 *
 * The "bottom half" of this routine is suffixed _bh
 *
 * -ah
 */

uns32b dca_bitstream_get_bh (dca_state_t * state, uns32b num_bits)
{
    uns32b result;

    num_bits -= state->bits_left;

    result = ((state->current_word << (32 - state->bits_left)) >>
              (32 - state->bits_left));

    if ( !state->word_mode && num_bits > 28 ) {
        bitstream_fill_current (state);
        result = (result << 28) | state->current_word;
        num_bits -= 28;
    }

    bitstream_fill_current (state);

    if ( state->word_mode )
    {
        if (num_bits != 0)
            result = (result << num_bits) |
                     (state->current_word >> (32 - num_bits));

        state->bits_left = 32 - num_bits;
    }
    else
    {
        if (num_bits != 0)
            result = (result << num_bits) |
                     (state->current_word >> (28 - num_bits));

        state->bits_left = 28 - num_bits;
    }

    return result;
}

dca_state_t * dca_init (uns32b mm_accel)
{
    dca_state_t * state;

    (void)mm_accel;
    state = (dca_state_t *) malloc (sizeof (dca_state_t));
    if (state == NULL)
        return NULL;

    memset (state, 0, sizeof(dca_state_t));

    state->downmixed = 1;

    return state;
}

static int syncinfo (dca_state_t * state, int * flags,
                     int * sample_rate, int * bit_rate, int * frame_length)
{
    int frame_size;

    /* Sync code */
    bitstream_get (state, 32);
    /* Frame type */
    bitstream_get (state, 1);
    /* Samples deficit */
    bitstream_get (state, 5);
    /* CRC present */
    bitstream_get (state, 1);

    *frame_length = (bitstream_get (state, 7) + 1) * 32;
    frame_size = bitstream_get (state, 14) + 1;
    if (!state->word_mode) frame_size = frame_size * 8 / 14 * 2;

    /* Audio channel arrangement */
    *flags = bitstream_get (state, 6);
    if (*flags > 63)
        return 0;

    *sample_rate = bitstream_get (state, 4);
    if ((size_t)*sample_rate >= sizeof (dca_sample_rates) / sizeof (int))
        return 0;
    *sample_rate = dca_sample_rates[ *sample_rate ];
    if (!*sample_rate) return 0;

    *bit_rate = bitstream_get (state, 5);
    if ((size_t)*bit_rate >= sizeof (dca_bit_rates) / sizeof (int))
        return 0;
    *bit_rate = dca_bit_rates[ *bit_rate ];
    if (!*bit_rate) return 0;

    /* LFE */
    bitstream_get (state, 10);
    if (bitstream_get (state, 2)) *flags |= DCA_LFE;

    return frame_size;
}

int dca_syncinfo (dca_state_t * state, uns8b * buf, int * flags,
                  int * sample_rate, int * bit_rate, int * frame_length)
{
    /*
     * Look for sync code
     */

    /* 14 bits and little endian bitstream */
    if (buf[0] == 0xff && buf[1] == 0x1f &&
        buf[2] == 0x00 && buf[3] == 0xe8 &&
        (buf[4] & 0xf0) == 0xf0 && buf[5] == 0x07)
    {
        int frame_size;
        dca_bitstream_init (state, buf, 0, 0);
        frame_size = syncinfo (state, flags, sample_rate,
                               bit_rate, frame_length);
        return frame_size;
    }

    /* 14 bits and big endian bitstream */
    if (buf[0] == 0x1f && buf[1] == 0xff &&
        buf[2] == 0xe8 && buf[3] == 0x00 &&
        buf[4] == 0x07 && (buf[5] & 0xf0) == 0xf0)
    {
        int frame_size;
        dca_bitstream_init (state, buf, 0, 1);
        frame_size = syncinfo (state, flags, sample_rate,
                               bit_rate, frame_length);
        return frame_size;
    }

    /* 16 bits and little endian bitstream */
    if (buf[0] == 0xfe && buf[1] == 0x7f &&
        buf[2] == 0x01 && buf[3] == 0x80)
    {
        int frame_size;
        dca_bitstream_init (state, buf, 1, 0);
        frame_size = syncinfo (state, flags, sample_rate,
                               bit_rate, frame_length);
        return frame_size;
    }

    /* 16 bits and big endian bitstream */
    if (buf[0] == 0x7f && buf[1] == 0xfe &&
        buf[2] == 0x80 && buf[3] == 0x01)
    {
        int frame_size;
        dca_bitstream_init (state, buf, 1, 1);
        frame_size = syncinfo (state, flags, sample_rate,
                               bit_rate, frame_length);
        return frame_size;
    }

    return 0;
}

void dca_frame (dca_state_t * state, uns8b * buf)
{
    dca_bitstream_init (state, buf, state->word_mode, state->bigendian_mode);

    /* Sync code */
    bitstream_get (state, 32);

    /* Frame header */
    state->frame_type = bitstream_get (state, 1);
    state->samples_deficit = bitstream_get (state, 5) + 1;
    state->crc_present = bitstream_get (state, 1);
    state->sample_blocks = bitstream_get (state, 7) + 1;
    state->frame_size = bitstream_get (state, 14) + 1;
    state->amode = bitstream_get (state, 6);
    state->sample_rate = bitstream_get (state, 4);
    state->bit_rate = bitstream_get (state, 5);

    state->downmix = bitstream_get (state, 1);
    state->dynrange = bitstream_get (state, 1);
    state->timestamp = bitstream_get (state, 1);
    state->aux_data = bitstream_get (state, 1);
    state->hdcd = bitstream_get (state, 1);
    state->ext_descr = bitstream_get (state, 3);
    state->ext_coding = bitstream_get (state, 1);
    state->aspf = bitstream_get (state, 1);
    state->lfe = bitstream_get (state, 2);
    state->predictor_history = bitstream_get (state, 1);

    /* TODO: check CRC */
    if (state->crc_present) state->header_crc = bitstream_get (state, 16);

    state->multirate_inter = bitstream_get (state, 1);
    state->version = bitstream_get (state, 4);
    state->copy_history = bitstream_get (state, 2);
    state->source_pcm_res = bitstream_get (state, 3);
    state->front_sum = bitstream_get (state, 1);
    state->surround_sum = bitstream_get (state, 1);
    state->dialog_norm = bitstream_get (state, 4);
}




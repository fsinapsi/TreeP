/*
 *  avilib.c
 *
 *  Copyright (C) Thomas Oestreich - June 2001
 *  multiple audio track support Copyright (C) 2002 Thomas Oestreich
 *  Version 1.1.0: Copyright (C) 2007-2008 Francesco Romani
 *
 *  Original code:
 *  Copyright (C) 1999 Rainer Johanni <Rainer@Johanni.de>
 *
 *  This file is part of transcode, a video stream processing tool
 *
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define PACKAGE "transcode"
#define VERSION "1.1.0"

#include <sys/stat.h>
#include "avilib.h"
#include "platform.h"

#define INFO_LIST

#ifdef MINGW
#define trp_off_t sig64b
#define trp_stat _stati64
#define trp_fstat _fstati64
#define ftello ftello64
#define fseeko fseeko64
#else
#define trp_off_t off_t
#define trp_stat stat
#define trp_fstat fstat
#endif

#define plat_read(fp,buf,n) trp_file_read_chars(fp,(uns8b *)(buf),(uns32b)(n))

enum {
    NEW_RIFF_THRES   = (1900*1024*1024), /* new riff chunk after XX MB */
    NR_IXNN_CHUNKS   = 32,               /* Maximum indices per stream */
    MAX_INFO_STRLEN  = 64,               /* XXX: ???                   */
    FRAME_RATE_SCALE = 1000000,          /* XXX: ???                   */
    HEADERBYTES      = 2048,             /* bytes for the header       */
};

/* AVI_MAX_LEN: The maximum length of an AVI file, we stay a bit below
    the 2GB limit (Remember: 2*10^9 is smaller than 2 GB) */
#define AVI_MAX_LEN (UINT_MAX-(1<<20)*16-HEADERBYTES)
#define PAD_EVEN(x) ( ((x)+1) & ~1 )

/*************************************************************************/
/* forward declarations                                                  */

static int avi_parse_input_file(avi_t *AVI, int getIndex);

/*************************************************************************/

/* The following variable indicates the kind of error */
static long AVI_errno = 0;

/*************************************************************************/

/* Copy n into dst as a 4 or 2 byte, little endian number.
   Should also work on big endian machines */

static void long2str(unsigned char *dst, int32_t n)
{
   dst[0] = (n    )&0xff;
   dst[1] = (n>> 8)&0xff;
   dst[2] = (n>>16)&0xff;
   dst[3] = (n>>24)&0xff;
}

/* Convert a string of 4 or 2 bytes to a number,
   also working on big endian machines */

static uns64b str2ullong(unsigned char *str)
{
   uns64b r = (str[0] | (str[1]<<8) | (str[2]<<16) | (str[3]<<24));
   uns64b s = (str[4] | (str[5]<<8) | (str[6]<<16) | (str[7]<<24));
   return ((s<<32)&0xffffffff00000000ULL)|(r&0xffffffffULL);
}

static uns32b str2ulong(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) | (str[2]<<16) | (str[3]<<24) );
}
static uns32b str2ushort(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) );
}

// bit 31 denotes a keyframe
static uns32b str2ulong_len (unsigned char *str)
{
   return str2ulong(str) & 0x7fffffff;
}

// if bit 31 is 0, its a keyframe
static uns32b str2ulong_key (unsigned char *str)
{
  uns32b c = str2ulong(str);
  return ((c & 0x80000000) ? 0 : 0x10);
}

/*************************************************************************/

/* Calculate audio sample size from number of bits and number of channels.
   This may have to be adjusted for eg. 12 bits and stereo */

static int avi_sampsize(avi_t *AVI, int j)
{
   int s;
   s = ((AVI->track[j].a_bits+7)/8)*AVI->track[j].a_chans;
   //   if(s==0) s=1; /* avoid possible zero divisions */
   if(s<4) s=4; /* avoid possible zero divisions */
   return s;
}

static int avi_add_index_entry(avi_t *AVI, const unsigned char *tag, long flags,
                               unsigned long pos, unsigned long len)
{
   void *ptr;

   if(AVI->n_idx>=AVI->max_idx) {
     ptr = plat_realloc((void *)AVI->idx,(AVI->max_idx+4096)*16);

     if(ptr == 0) {
       AVI_errno = AVI_ERR_NO_MEM;
       return -1;
     }
     AVI->max_idx += 4096;
     AVI->idx = (unsigned char((*)[16]) ) ptr;
   }

   /* Add index entry */

   //   fprintf(stderr, "INDEX %s %ld %lu %lu\n", tag, flags, pos, len);

   memcpy(AVI->idx[AVI->n_idx],tag,4);
   long2str(AVI->idx[AVI->n_idx]+ 4,flags);
   long2str(AVI->idx[AVI->n_idx]+ 8, pos);
   long2str(AVI->idx[AVI->n_idx]+12, len);

   /* Update counter */

   AVI->n_idx++;

   if(len>AVI->max_len) AVI->max_len=len;

   return 0;
}

/*

static int valid_info_tag(char *c)
{
    if      (!strncmp(c, "IARL", 4)) return 1;
    else if (!strncmp(c, "IART", 4)) return 1;
    else if (!strncmp(c, "ICMS", 4)) return 1;
    else if (!strncmp(c, "ICMT", 4)) return 1;
    else if (!strncmp(c, "ICOP", 4)) return 1;
    else if (!strncmp(c, "ICRD", 4)) return 1;
    else if (!strncmp(c, "ICRP", 4)) return 1;
    else if (!strncmp(c, "IDIM", 4)) return 1;
    else if (!strncmp(c, "IDPI", 4)) return 1;
    else if (!strncmp(c, "IENG", 4)) return 1;
    else if (!strncmp(c, "IGNR", 4)) return 1;
    else if (!strncmp(c, "IKEY", 4)) return 1;
    else if (!strncmp(c, "ILGT", 4)) return 1;
    else if (!strncmp(c, "IMED", 4)) return 1;
    else if (!strncmp(c, "INAM", 4)) return 1;
    else if (!strncmp(c, "IPLT", 4)) return 1;
    else if (!strncmp(c, "IPRD", 4)) return 1;
    else if (!strncmp(c, "ISBJ", 4)) return 1;
    else if (!strncmp(c, "ISHP", 4)) return 1;
    else if (!strncmp(c, "ISRC", 4)) return 1;
    else if (!strncmp(c, "ISRF", 4)) return 1;
    else if (!strncmp(c, "ITCH", 4)) return 1;
    else return 0;

    return 0;
}

*/

//SLM
#ifndef S_IRUSR
#define S_IRWXU       00700       /* read, write, execute: owner */
#define S_IRUSR       00400       /* read permission: owner */
#define S_IWUSR       00200       /* write permission: owner */
#define S_IXUSR       00100       /* execute permission: owner */
#define S_IRWXG       00070       /* read, write, execute: group */
#define S_IRGRP       00040       /* read permission: group */
#define S_IWGRP       00020       /* write permission: group */
#define S_IXGRP       00010       /* execute permission: group */
#define S_IRWXO       00007       /* read, write, execute: other */
#define S_IROTH       00004       /* read permission: other */
#define S_IWOTH       00002       /* write permission: other */
#define S_IXOTH       00001       /* execute permission: other */
#endif

int AVI_set_audio_track(avi_t *AVI, int track)
{

  if(track < 0 || track + 1 > AVI->anum) return(-1);

  //this info is not written to file anyway
  AVI->aptr=track;
  return 0;
}

int AVI_get_audio_track(avi_t *AVI)
{
    return(AVI->aptr);
}

long AVI_get_audio_vbr(avi_t *AVI)
{
    return(AVI->track[AVI->aptr].a_vbr);
}

/*******************************************************************
 *                                                                 *
 *    Utilities for reading video and audio from an AVI File       *
 *                                                                 *
 *******************************************************************/

int AVI_close(avi_t *AVI)
{
    int j, k, ret = 0;

    fclose(AVI->fdes);

    if (AVI->idx)
        plat_free(AVI->idx);
    if (AVI->video_index)
        plat_free(AVI->video_index);

    /*
     bug fixed - Frank Sinapsi - 2009-12-22
     */
    if (AVI->video_superindex) {
        if (AVI->video_superindex->stdindex) {
            for (j = 0; j < NR_IXNN_CHUNKS; j++) {
                if (AVI->video_superindex->stdindex[j]) {
                    if (AVI->video_superindex->stdindex[j]->aIndex) {
                        plat_free(AVI->video_superindex->stdindex[j]->aIndex);
                    }
                    plat_free(AVI->video_superindex->stdindex[j]);
                }
            }
            plat_free(AVI->video_superindex->stdindex);
        }
        if (AVI->video_superindex->aIndex)
            plat_free(AVI->video_superindex->aIndex);
        plat_free(AVI->video_superindex);
    }

    for (j = 0; j < AVI->anum; j++) {
        if (AVI->track[j].audio_index)
            plat_free(AVI->track[j].audio_index);
        if (AVI->track[j].audio_superindex) {
            // shortcut
            avisuperindex_chunk *a = AVI->track[j].audio_superindex;
                for (k = 0; k < NR_IXNN_CHUNKS; k++) {
                    if (a->stdindex && a->stdindex[k]) {
                            if (a->stdindex[k]->aIndex) {
                                plat_free(a->stdindex[k]->aIndex);
                            }
                            plat_free(a->stdindex[k]);
                    }
                }
                if (a->stdindex)
                plat_free(a->stdindex);
                if (a->aIndex)
                plat_free(a->aIndex);
            plat_free(a);
        }
    }

    if (AVI->bitmap_info_header)
        plat_free(AVI->bitmap_info_header);
    for (j = 0; j < AVI->anum; j++)
        if (AVI->wave_format_ex[j])
            plat_free(AVI->wave_format_ex[j]);

    plat_free(AVI);
    AVI = NULL;

    return ret;
}

#define ERR_EXIT(x) do { \
   AVI_close(AVI); \
   AVI_errno = x; \
   return 0; \
} while (0)

avi_t *AVI_open_input_file(const char *filename, int getIndex)
{
    FILE *fp = trp_fopen(filename,"rb");

    if (fp == NULL) {
        AVI_errno = AVI_ERR_OPEN;
        return NULL;
    }

    avi_t *AVI = plat_zalloc(sizeof(avi_t));
    if (AVI == NULL) {
        AVI_errno = AVI_ERR_NO_MEM;
        return NULL;
    }

    AVI->mode = AVI_MODE_READ; /* open for reading */

    // file alread open
    AVI->fdes = fp;

    AVI_errno = 0;
    avi_parse_input_file(AVI, getIndex);

    if (AVI != NULL && !AVI_errno) {
        AVI->aptr = 0; //reset
    }

    return (AVI_errno) ?NULL :AVI;
}

static uns8b *avi_build_audio_superindex(avisuperindex_chunk *si, uns8b *a)
{
    int j = 0;

    memcpy(si->fcc, a, 4);              a += 4;
    si->dwSize = str2ulong(a);          a += 4;
    si->wLongsPerEntry = str2ushort(a); a += 2;
    si->bIndexSubType = *a;             a += 1;
    si->bIndexType = *a;                a += 1;
    si->nEntriesInUse = str2ulong(a);   a += 4;
    memcpy(si->dwChunkId, a, 4);        a += 4;
    // 3 * reserved
    a += (3 * 4);

    if (si->bIndexSubType != 0) {
        plat_log_send(PLAT_LOG_WARNING, __FILE__, "Invalid Header, bIndexSubType != 0");
    }

    si->aIndex = plat_zalloc(si->wLongsPerEntry * si->nEntriesInUse * sizeof(uns32b));
    // position of ix## chunks
    for (j=0; j < si->nEntriesInUse; ++j) {
        si->aIndex[j].qwOffset   = str2ullong(a); a += 8;
        si->aIndex[j].dwSize     = str2ulong(a);  a += 4;
        si->aIndex[j].dwDuration = str2ulong(a);  a += 4;
#ifdef DEBUG_ODML
        printf("[%d] offset=0x%llx size=0x%lx duration=%lu\n", j,
                       (unsigned long long)si->aIndex[j].qwOffset,
                       (unsigned long)si->aIndex[j].dwSize,
                       (unsigned long)si->aIndex[j].dwDuration);
#endif
    }
#ifdef DEBUG_ODML
    printf("  FOURCC         \"%.4s\"\n", si->fcc);
    printf("  LEN            \"%ld\"\n",  si->dwSize);
    printf("  wLongsPerEntry \"%d\"\n",   si->wLongsPerEntry);
    printf("  bIndexSubType  \"%d\"\n",   si->bIndexSubType);
    printf("  bIndexType     \"%d\"\n",   si->bIndexType);
    printf("  nEntriesInUse  \"%ld\"\n",  si->nEntriesInUse);
    printf("  dwChunkId      \"%.4s\"\n", si->dwChunkId[0]);
    printf("--\n");
#endif
    return a;
}

static int avi_parse_input_file(avi_t *AVI, int getIndex)
{
  long i, rate, scale, idx_type;
  uns8b *hdrl_data = NULL;
  long header_offset = 0, hdrl_len = 0;
  long nvi, nai[AVI_MAX_TRACKS], ioff;
  long tot[AVI_MAX_TRACKS];
  int j, num_stream = 0;
  int lasttag = 0;
  int vids_strh_seen = 0;
  int vids_strf_seen = 0;
  int auds_strh_seen = 0;
  //  int auds_strf_seen = 0;
  char data[256];
  trp_off_t oldpos=-1, newpos=-1, n;

  /* Read first 12 bytes and check that this is an AVI file */

   if( plat_read(AVI->fdes,data,12) != 12 ) ERR_EXIT(AVI_ERR_READ);

   if( strncasecmp(data  ,"RIFF",4) !=0 ||
       strncasecmp(data+8,"AVI ",4) !=0 ) ERR_EXIT(AVI_ERR_NO_AVI);

   /* Go through the AVI file and extract the header list,
      the start position of the 'movi' list and an optionally
      present idx1 tag */

   while (1) {
      if( plat_read(AVI->fdes,data,8) != 8 ) break; /* We assume it's EOF */
      newpos=ftello(AVI->fdes);
      if(oldpos==newpos) {
              /* This is a broken AVI stream... */
              return -1;
      }
      oldpos=newpos;

      n = str2ulong((unsigned char *)data+4);
      n = PAD_EVEN(n);

      if(strncasecmp(data,"LIST",4) == 0)
      {
         if( plat_read(AVI->fdes,data,4) != 4 ) ERR_EXIT(AVI_ERR_READ);
         n -= 4;
         if(strncasecmp(data,"hdrl",4) == 0)
         {
            hdrl_len = n;
            hdrl_data = plat_malloc(n);
            if(hdrl_data==0) ERR_EXIT(AVI_ERR_NO_MEM);

            // offset of header

            header_offset = ftello(AVI->fdes);

            if( plat_read(AVI->fdes,(char *)hdrl_data,n) != n ) ERR_EXIT(AVI_ERR_READ);
         }
         else if(strncasecmp(data,"movi",4) == 0)
         {
            AVI->movi_start = ftello(AVI->fdes);
            if (fseeko(AVI->fdes,n,SEEK_CUR)) break;
         }
         else
            if (fseeko(AVI->fdes,n,SEEK_CUR)) break;
      }
      else if(strncasecmp(data,"idx1",4) == 0)
      {
         /* n must be a multiple of 16, but the reading does not
            break if this is not the case */

         AVI->n_idx = AVI->max_idx = n/16;
         AVI->idx = (unsigned  char((*)[16]) ) plat_malloc(n);
         if(AVI->idx==0) ERR_EXIT(AVI_ERR_NO_MEM);
         if(plat_read(AVI->fdes, (char *) AVI->idx, n) != n ) {
             free ( AVI->idx); AVI->idx=NULL;
             AVI->n_idx = 0;
         }
      }
      else
         fseeko(AVI->fdes,n,SEEK_CUR);
   }

   if(!hdrl_data      ) ERR_EXIT(AVI_ERR_NO_HDRL);
   if(!AVI->movi_start) ERR_EXIT(AVI_ERR_NO_MOVI);

   /* Interpret the header list */

   for(i=0;i<hdrl_len;)
   {
      /* List tags are completly ignored */

#ifdef DEBUG_ODML
      printf("TAG %c%c%c%c\n", (hdrl_data+i)[0], (hdrl_data+i)[1], (hdrl_data+i)[2], (hdrl_data+i)[3]);
#endif

      if(strncasecmp((char *)hdrl_data+i,"LIST",4)==0) { i+= 12; continue; }

      n = str2ulong(hdrl_data+i+4);
      n = PAD_EVEN(n);

      /* Interpret the tag and its args */

      if(strncasecmp((char *)hdrl_data+i,"strh",4)==0)
      {
         i += 8;
#ifdef DEBUG_ODML
         printf("TAG   %c%c%c%c\n", (hdrl_data+i)[0], (hdrl_data+i)[1], (hdrl_data+i)[2], (hdrl_data+i)[3]);
#endif
         if(strncasecmp((char *)hdrl_data+i,"vids",4) == 0 && !vids_strh_seen)
         {
            memcpy(AVI->compressor,hdrl_data+i+4,4);
            AVI->compressor[4] = 0;

            // ThOe
            AVI->v_codech_off = header_offset + i+4;

            scale = str2ulong(hdrl_data+i+20);
            rate  = str2ulong(hdrl_data+i+24);
            if(scale!=0) AVI->fps = (double)rate/(double)scale;
            AVI->v_scale = scale;                     /* Frank Sinapsi - 2009-12-22 */
            AVI->v_rate = rate;                       /* Frank Sinapsi - 2009-12-22 */
            AVI->v_start = str2ulong(hdrl_data+i+28); /* Frank Sinapsi - 2009-12-22 */
            AVI->video_frames = str2ulong(hdrl_data+i+32);
            AVI->video_strn = num_stream;
            AVI->max_len = 0;
            vids_strh_seen = 1;
            lasttag = 1; /* vids */
         }
         else if (strncasecmp ((char *)hdrl_data+i,"auds",4) ==0 && ! auds_strh_seen)
         {

           //inc audio tracks
           AVI->aptr=AVI->anum;
           ++AVI->anum;

           if(AVI->anum > AVI_MAX_TRACKS) {
         plat_log_send(PLAT_LOG_ERROR, __FILE__, "only %d audio tracks supported", AVI_MAX_TRACKS);
             return(-1);
           }

           AVI->track[AVI->aptr].audio_bytes = str2ulong(hdrl_data+i+32)*avi_sampsize(AVI, 0);
           AVI->track[AVI->aptr].audio_strn = num_stream;

           // if samplesize==0 -> vbr
           AVI->track[AVI->aptr].a_vbr = !str2ulong(hdrl_data+i+44);

           AVI->track[AVI->aptr].a_scale = str2ulong(hdrl_data+i+20); /* Frank Sinapsi - 2009-12-22 */
           AVI->track[AVI->aptr].padrate = str2ulong(hdrl_data+i+24);
           AVI->track[AVI->aptr].a_start = str2ulong(hdrl_data+i+28); /* Frank Sinapsi - 2009-12-22 */

           //	   auds_strh_seen = 1;
           lasttag = 2; /* auds */

           // ThOe
           AVI->track[AVI->aptr].a_codech_off = header_offset + i;

         }
         else if (strncasecmp (hdrl_data+i,"iavs",4) ==0 && ! auds_strh_seen) {
         plat_log_send(PLAT_LOG_ERROR, __FILE__, "DV AVI Type 1 no supported");
             return (-1);
         }
         else
            lasttag = 0;
         num_stream++;
      }
      else if(strncasecmp(hdrl_data+i,"dmlh",4) == 0) {
          AVI->total_frames = str2ulong(hdrl_data+i+8);
#ifdef DEBUG_ODML
         fprintf(stderr, "real number of frames %d\n", AVI->total_frames);
#endif
         i += 8;
      }
      else if(strncasecmp((char *)hdrl_data+i,"strf",4)==0)
      {
         i += 8;
         if(lasttag == 1)
         {
            alBITMAPINFOHEADER bih;

            memcpy(&bih, hdrl_data + i, sizeof(alBITMAPINFOHEADER));
            AVI->bitmap_info_header = plat_malloc(str2ulong((unsigned char *)&bih.bi_size));
            if (AVI->bitmap_info_header != NULL)
              memcpy(AVI->bitmap_info_header, hdrl_data + i,
                     str2ulong((unsigned char *)&bih.bi_size));

            AVI->width  = str2ulong(hdrl_data+i+4);
            AVI->height = str2ulong(hdrl_data+i+8);
                    vids_strf_seen = 1;
            //ThOe
            AVI->v_codecf_off = header_offset + i+16;

            memcpy(AVI->compressor2, hdrl_data+i+16, 4);
            AVI->compressor2[4] = 0;

         }
         else if(lasttag == 2)
         {
            alWAVEFORMATEX *wfe;
            char *nwfe;
            int wfes;

            if ((hdrl_len - i) < sizeof(alWAVEFORMATEX))
              wfes = hdrl_len - i;
            else
              wfes = sizeof(alWAVEFORMATEX);
            wfe = plat_zalloc(sizeof(alWAVEFORMATEX));
            if (wfe != NULL) {
              memcpy(wfe, hdrl_data + i, wfes);
              if (str2ushort((unsigned char *)&wfe->cb_size) != 0) {
                nwfe = plat_realloc(wfe, sizeof(alWAVEFORMATEX) +
                          str2ushort((unsigned char *)&wfe->cb_size));
                if (nwfe != 0) {
                  trp_off_t lpos = ftello(AVI->fdes);
                  fseeko(AVI->fdes, header_offset + i + sizeof(alWAVEFORMATEX),SEEK_SET);
                  wfe = (alWAVEFORMATEX *)nwfe;
                  nwfe = &nwfe[sizeof(alWAVEFORMATEX)];
                  plat_read(AVI->fdes, nwfe,
                           str2ushort((unsigned char *)&wfe->cb_size));
                  fseeko(AVI->fdes, lpos, SEEK_SET);
                }
              }
              AVI->wave_format_ex[AVI->aptr] = wfe;
            }

            AVI->track[AVI->aptr].a_fmt   = str2ushort(hdrl_data+i  );

            //ThOe
            AVI->track[AVI->aptr].a_codecf_off = header_offset + i;

            AVI->track[AVI->aptr].a_chans = str2ushort(hdrl_data+i+2);
            AVI->track[AVI->aptr].a_rate  = str2ulong (hdrl_data+i+4);
            //ThOe: read mp3bitrate
            AVI->track[AVI->aptr].mp3rate = 8*str2ulong(hdrl_data+i+8)/1000;
            //:ThOe
            AVI->track[AVI->aptr].a_bits  = str2ushort(hdrl_data+i+14);
            //            auds_strf_seen = 1;
         }
      }
      else if(strncasecmp(hdrl_data+i,"indx",4) == 0) {
         char *a;
         int j;

         if(lasttag == 1) // V I D E O
         {

            a = hdrl_data+i;

            AVI->video_superindex = plat_zalloc(sizeof(avisuperindex_chunk));
            memcpy (AVI->video_superindex->fcc, a, 4);             a += 4;
            AVI->video_superindex->dwSize = str2ulong(a);          a += 4;
            AVI->video_superindex->wLongsPerEntry = str2ushort(a); a += 2;
            AVI->video_superindex->bIndexSubType = *a;             a += 1;
            AVI->video_superindex->bIndexType = *a;                a += 1;
            AVI->video_superindex->nEntriesInUse = str2ulong(a);   a += 4;
            memcpy (AVI->video_superindex->dwChunkId, a, 4);       a += 4;

            // 3 * reserved
            a += 4; a += 4; a += 4;

            if (AVI->video_superindex->bIndexSubType != 0) {
        plat_log_send(PLAT_LOG_WARNING, __FILE__, "Invalid Header, bIndexSubType != 0");
        }

            AVI->video_superindex->aIndex =
               plat_malloc(AVI->video_superindex->wLongsPerEntry * AVI->video_superindex->nEntriesInUse * sizeof(uns32b));

            // position of ix## chunks
            for (j=0; j<AVI->video_superindex->nEntriesInUse; ++j) {
               AVI->video_superindex->aIndex[j].qwOffset = str2ullong (a);  a += 8;
               AVI->video_superindex->aIndex[j].dwSize = str2ulong (a);     a += 4;
               AVI->video_superindex->aIndex[j].dwDuration = str2ulong (a); a += 4;

#ifdef DEBUG_ODML
               printf("[%d] 0x%llx 0x%lx %lu\n", j,
                       (unsigned long long)AVI->video_superindex->aIndex[j].qwOffset,
                       (unsigned long)AVI->video_superindex->aIndex[j].dwSize,
                       (unsigned long)AVI->video_superindex->aIndex[j].dwDuration);
#endif
            }

#ifdef DEBUG_ODML
            printf("FOURCC \"%c%c%c%c\"\n", AVI->video_superindex->fcc[0], AVI->video_superindex->fcc[1],
                                            AVI->video_superindex->fcc[2], AVI->video_superindex->fcc[3]);
            printf("LEN \"%ld\"\n", (long)AVI->video_superindex->dwSize);
            printf("wLongsPerEntry \"%d\"\n", AVI->video_superindex->wLongsPerEntry);
            printf("bIndexSubType \"%d\"\n", AVI->video_superindex->bIndexSubType);
            printf("bIndexType \"%d\"\n", AVI->video_superindex->bIndexType);
            printf("nEntriesInUse \"%ld\"\n", (long)AVI->video_superindex->nEntriesInUse);
            printf("dwChunkId \"%c%c%c%c\"\n", AVI->video_superindex->dwChunkId[0], AVI->video_superindex->dwChunkId[1],
                                               AVI->video_superindex->dwChunkId[2], AVI->video_superindex->dwChunkId[3]);
            printf("--\n");
#endif

            AVI->is_opendml = 1;

         }
         else if(lasttag == 2) // A U D I O
         {
            a = hdrl_data+i;

            AVI->track[AVI->aptr].audio_superindex = plat_zalloc(sizeof(avisuperindex_chunk));

        a = avi_build_audio_superindex(AVI->track[AVI->aptr].audio_superindex, a);
         }
         i += 8;
      }
      else if((strncasecmp(hdrl_data+i,"JUNK",4) == 0) ||
              (strncasecmp(hdrl_data+i,"strn",4) == 0) ||
              (strncasecmp(hdrl_data+i,"vprp",4) == 0)){
         i += 8;
         // do not reset lasttag
      } else
      {
         i += 8;
         lasttag = 0;
      }
      //printf("adding %ld bytes\n", (long int)n);

      i += n;
   }

   plat_free(hdrl_data);

   if(!vids_strh_seen || !vids_strf_seen) ERR_EXIT(AVI_ERR_NO_VIDS);

   AVI->video_tag[0] = AVI->video_strn/10 + '0';
   AVI->video_tag[1] = AVI->video_strn%10 + '0';
   AVI->video_tag[2] = 'd';
   AVI->video_tag[3] = 'b';

   /* Audio tag is set to "99wb" if no audio present */
   if(!AVI->track[0].a_chans) AVI->track[0].audio_strn = 99;

   {
     int i=0;
     for(j=0; j<AVI->anum+1; ++j) {
       if (j == AVI->video_strn) continue;
       AVI->track[i].audio_tag[0] = j/10 + '0';
       AVI->track[i].audio_tag[1] = j%10 + '0';
       AVI->track[i].audio_tag[2] = 'w';
       AVI->track[i].audio_tag[3] = 'b';
       ++i;
     }
   }

   fseeko(AVI->fdes,AVI->movi_start,SEEK_SET);

   if(!getIndex) return(0);

   /* if the file has an idx1, check if this is relative
      to the start of the file or to the start of the movi list */

   idx_type = 0;

   if(AVI->idx)
   {
      trp_off_t pos, len;

      /* Search the first videoframe in the idx1 and look where
         it is in the file */

      for(i=0;i<AVI->n_idx;i++)
         if( strncasecmp((char *)AVI->idx[i],(char *)AVI->video_tag,3)==0 ) break;
      if(i>=AVI->n_idx) ERR_EXIT(AVI_ERR_NO_VIDS);

      pos = str2ulong(AVI->idx[i]+ 8);
      len = str2ulong(AVI->idx[i]+12);

      fseeko(AVI->fdes,pos,SEEK_SET);
      if(plat_read(AVI->fdes,data,8)!=8) ERR_EXIT(AVI_ERR_READ);
      if( strncasecmp(data,(char *)AVI->idx[i],4)==0 && str2ulong((unsigned char *)data+4)==len )
      {
         idx_type = 1; /* Index from start of file */
      }
      else
      {
         fseeko(AVI->fdes,pos+AVI->movi_start-4,SEEK_SET);
         if(plat_read(AVI->fdes,data,8)!=8) ERR_EXIT(AVI_ERR_READ);
         if( strncasecmp(data,(char *)AVI->idx[i],4)==0 && str2ulong((unsigned char *)data+4)==len )
         {
            idx_type = 2; /* Index from start of movi list */
         }
      }
      /* idx_type remains 0 if neither of the two tests above succeeds */
   }

   if(idx_type == 0 && !AVI->is_opendml && !AVI->total_frames)
   {

      return 0; /* Frank Sinapsi */
#if 0
      /* we must search through the file to get the index */
      fseeko(AVI->fdes, AVI->movi_start, SEEK_SET);

      AVI->n_idx = 0;

      while(1)
      {
         if( plat_read(AVI->fdes,data,8) != 8 ) break;
         n = str2ulong((unsigned char *)data+4);

         /* The movi list may contain sub-lists, ignore them */

         if(strncasecmp(data,"LIST",4)==0)
         {
            fseeko(AVI->fdes,4,SEEK_CUR);
            continue;
         }

         /* Check if we got a tag ##db, ##dc or ##wb */

         if( ( (data[2]=='d' || data[2]=='D') &&
               (data[3]=='b' || data[3]=='B' || data[3]=='c' || data[3]=='C') )
             || ( (data[2]=='w' || data[2]=='W') &&
                  (data[3]=='b' || data[3]=='B') ) )
           {
           avi_add_index_entry(AVI,(unsigned char *)data,0,ftello(AVI->fdes)-8,n);
         }

         fseeko(AVI->fdes,PAD_EVEN(n),SEEK_CUR);
      }
      idx_type = 1;
#endif
   }

   // ************************
   // OPENDML
   // ************************

   // read extended index chunks
   if (AVI->is_opendml) {
      uns64b offset = 0;
      int hdrl_len = 4+4+2+1+1+4+4+8+4;
      char *en, *chunk_start;
      int k = 0, audtr = 0;
      uns32b nrEntries = 0;

      AVI->video_index = NULL;

      nvi = 0;
      for(audtr=0; audtr<AVI->anum; ++audtr) nai[audtr] = tot[audtr] = 0;

      // ************************
      // VIDEO
      // ************************

      for (j=0; j<AVI->video_superindex->nEntriesInUse; j++) {

         // read from file
         chunk_start = en = plat_malloc (AVI->video_superindex->aIndex[j].dwSize+hdrl_len);

         if (fseeko(AVI->fdes, AVI->video_superindex->aIndex[j].qwOffset, SEEK_SET)) {
        plat_log_send(PLAT_LOG_WARNING, __FILE__, "cannot seek to 0x%llx",
                    (unsigned long long)AVI->video_superindex->aIndex[j].qwOffset);
            plat_free(chunk_start);
            continue;
         }

         if (plat_read(AVI->fdes, en, AVI->video_superindex->aIndex[j].dwSize+hdrl_len) != AVI->video_superindex->aIndex[j].dwSize+hdrl_len) {
        plat_log_send(PLAT_LOG_WARNING, __FILE__,
                     "cannot read from offset 0x%llx %ld bytes; broken (incomplete) file?",
                    (unsigned long long)AVI->video_superindex->aIndex[j].qwOffset,
                    (unsigned long)AVI->video_superindex->aIndex[j].dwSize+hdrl_len);
            plat_free(chunk_start);
            continue;
         }

         nrEntries = str2ulong(en + 12);
#ifdef DEBUG_ODML
         //printf("[%d:0] Video nrEntries %ld\n", j, nrEntries);
#endif
         offset = str2ullong(en + 20);

         // skip header
         en += hdrl_len;
         nvi += nrEntries;
         AVI->video_index = plat_realloc(AVI->video_index, nvi * sizeof(video_index_entry));
         if (!AVI->video_index) {
         plat_log_send(PLAT_LOG_ERROR, __FILE__, "out of mem (size = %ld)",
                       nvi * sizeof(video_index_entry));
                 exit(1); // XXX XXX XXX
         }

         while (k < nvi) {

            AVI->video_index[k].pos = offset + str2ulong(en); en += 4;
            AVI->video_index[k].len = str2ulong_len(en);
            AVI->v_streamsize += AVI->video_index[k].len; /* Frank Sinapsi - 2009-12-22 */
            AVI->video_index[k].key = str2ulong_key(en); en += 4;

            // completely empty chunk
            if (AVI->video_index[k].pos-offset == 0 && AVI->video_index[k].len == 0) {
                k--;
                nvi--;
            }

#ifdef DEBUG_ODML
            /*
            printf("[%d] POS 0x%llX len=%d key=%s offset (%llx) (%ld)\n", k,
                  AVI->video_index[k].pos,
                  (int)AVI->video_index[k].len,
                  AVI->video_index[k].key?"yes":"no ", offset,
                  AVI->video_superindex->aIndex[j].dwSize);
                  */
#endif

            k++;
         }

         plat_free(chunk_start);
      }

      AVI->video_frames = nvi;
      // this should deal with broken 'rec ' odml files.
      if (AVI->video_frames == 0) {
          AVI->is_opendml=0;
          goto multiple_riff;
      }

      // ************************
      // AUDIO
      // ************************

      for(audtr=0; audtr<AVI->anum; ++audtr) {

         k = 0;
         if (!AVI->track[audtr].audio_superindex) {
               plat_log_send(PLAT_LOG_WARNING, __FILE__, "cannot read audio index for track %d", audtr);
               continue;
         }
         for (j=0; j<AVI->track[audtr].audio_superindex->nEntriesInUse; j++) {

            // read from file
            chunk_start = en = plat_malloc(AVI->track[audtr].audio_superindex->aIndex[j].dwSize+hdrl_len);

            if (fseeko(AVI->fdes, AVI->track[audtr].audio_superindex->aIndex[j].qwOffset, SEEK_SET)) {
               plat_log_send(PLAT_LOG_WARNING, __FILE__,
                         "cannot seek to 0x%llx",
                         (unsigned long long)AVI->track[audtr].audio_superindex->aIndex[j].qwOffset);
               plat_free(chunk_start);
               continue;
            }

            if (plat_read(AVI->fdes, en, AVI->track[audtr].audio_superindex->aIndex[j].dwSize+hdrl_len) != AVI->track[audtr].audio_superindex->aIndex[j].dwSize+hdrl_len) {
               plat_log_send(PLAT_LOG_WARNING, __FILE__,
                         "cannot read from offset 0x%llx; broken (incomplete) file?",
                         (unsigned long long) AVI->track[audtr].audio_superindex->aIndex[j].qwOffset);
               plat_free(chunk_start);
               continue;
            }

            nrEntries = str2ulong(en + 12);
            //if (nrEntries > 50) nrEntries = 2; // XXX
#ifdef DEBUG_ODML
            //printf("[%d:%d] Audio nrEntries %ld\n", j, audtr, nrEntries);
#endif
            offset = str2ullong(en + 20);

            // skip header
            en += hdrl_len;
            nai[audtr] += nrEntries;
            AVI->track[audtr].audio_index = plat_realloc(AVI->track[audtr].audio_index, nai[audtr] * sizeof(audio_index_entry));

            while (k < nai[audtr]) {

               AVI->track[audtr].audio_index[k].pos = offset + str2ulong(en); en += 4;
               AVI->track[audtr].audio_index[k].len = str2ulong_len(en); en += 4;
               AVI->track[audtr].audio_index[k].tot = tot[audtr];
               tot[audtr] += AVI->track[audtr].audio_index[k].len;

#ifdef DEBUG_ODML
               /*
                  printf("[%d:%d] POS 0x%llX len=%d offset (%llx) (%ld)\n", k, audtr,
                  AVI->track[audtr].audio_index[k].pos,
                  (int)AVI->track[audtr].audio_index[k].len,
                  offset, AVI->track[audtr].audio_superindex->aIndex[j].dwSize);
                  */
#endif

               ++k;
            }

            plat_free(chunk_start);
         }

         AVI->track[audtr].audio_chunks = nai[audtr];
         AVI->track[audtr].audio_bytes = tot[audtr];
      }
   } // is opendml

   else if (AVI->total_frames && !AVI->is_opendml && idx_type==0) {

   // *********************
   // MULTIPLE RIFF CHUNKS (and no index)
   // *********************

      long aud_chunks = 0;

      return 0; /* Frank Sinapsi */

multiple_riff:

      fseeko(AVI->fdes, AVI->movi_start, SEEK_SET);

      AVI->n_idx = 0;

      plat_log_send(PLAT_LOG_INFO, __FILE__, "Reconstructing index...");

      // Number of frames; only one audio track supported
      nvi = AVI->video_frames = AVI->total_frames;
      nai[0] = AVI->track[0].audio_chunks = AVI->total_frames;
      for(j=1; j<AVI->anum; ++j) AVI->track[j].audio_chunks = 0;

      AVI->video_index = plat_malloc(nvi*sizeof(video_index_entry));

      if(AVI->video_index==0) ERR_EXIT(AVI_ERR_NO_MEM);

      for(j=0; j<AVI->anum; ++j) {
          if(AVI->track[j].audio_chunks) {
              AVI->track[j].audio_index = plat_zalloc((nai[j]+1)*sizeof(audio_index_entry));
              if(AVI->track[j].audio_index==0) ERR_EXIT(AVI_ERR_NO_MEM);
          }
      }

      nvi = 0;
      for(j=0; j<AVI->anum; ++j) nai[j] = tot[j] = 0;

      aud_chunks = AVI->total_frames;

      while(1)
      {
         if (nvi >= AVI->total_frames) break;

         if( plat_read(AVI->fdes,data,8) != 8 ) break;
         n = str2ulong((unsigned char *)data+4);

         j=0;

         if (aud_chunks - nai[j] -1 <= 0) {
             aud_chunks += AVI->total_frames;
             AVI->track[j].audio_index = plat_realloc( AVI->track[j].audio_index, (aud_chunks+1)*sizeof(audio_index_entry));
             if (!AVI->track[j].audio_index) {
         plat_log_send(PLAT_LOG_ERROR, __FILE__, "Internal error -- no mem");
                 AVI_errno = AVI_ERR_NO_MEM;
                 return -1;
             }
         }

         /* Check if we got a tag ##db, ##dc or ##wb */

         // VIDEO
         if(
             (data[0]=='0' || data[1]=='0') &&
             (data[2]=='d' || data[2]=='D') &&
             (data[3]=='b' || data[3]=='B' || data[3]=='c' || data[3]=='C') ) {

             AVI->video_index[nvi].key = 0x0;
             AVI->video_index[nvi].pos = ftello(AVI->fdes);
             AVI->video_index[nvi].len = n;
             AVI->v_streamsize += n; /* Frank Sinapsi - 2009-12-22 */

             /*
             fprintf(stderr, "Frame %ld pos %lld len %lld key %ld\n",
                     nvi, AVI->video_index[nvi].pos,  AVI->video_index[nvi].len, (long)AVI->video_index[nvi].key);
                     */
             nvi++;
             fseeko(AVI->fdes,PAD_EVEN(n),SEEK_CUR);
         }

         //AUDIO
         else if(
                 (data[0]=='0' || data[1]=='1') &&
                 (data[2]=='w' || data[2]=='W') &&
             (data[3]=='b' || data[3]=='B') ) {

                AVI->track[j].audio_index[nai[j]].pos = ftello(AVI->fdes);
                AVI->track[j].audio_index[nai[j]].len = n;
                AVI->track[j].audio_index[nai[j]].tot = tot[j];
                tot[j] += AVI->track[j].audio_index[nai[j]].len;
                nai[j]++;

                fseeko(AVI->fdes,PAD_EVEN(n),SEEK_CUR);
         }
         else {
            fseeko(AVI->fdes,-4,SEEK_CUR);
         }

      }
      if (nvi < AVI->total_frames) {
          plat_log_send(PLAT_LOG_WARNING, __FILE__,
                        "Uh? Some frames seems missing (%ld/%d)",
                        nvi,  AVI->total_frames);
      }

      AVI->video_frames = nvi;
      AVI->track[0].audio_chunks = nai[0];

      for(j=0; j<AVI->anum; ++j) AVI->track[j].audio_bytes = tot[j];
      idx_type = 1;
      plat_log_send(PLAT_LOG_INFO, __FILE__,
                    "done. nvi=%ld nai=%ld tot=%ld", nvi, nai[0], tot[0]);

   } // total_frames but no indx chunk (xawtv does this)

   else

   {
   // ******************
   // NO OPENDML
   // ******************

   /* Now generate the video index and audio index arrays */

   nvi = 0;
   for(j=0; j<AVI->anum; ++j) nai[j] = 0;

   for(i=0;i<AVI->n_idx;i++) {

     if(strncasecmp((char *)AVI->idx[i],AVI->video_tag,3) == 0) nvi++;

     for(j=0; j<AVI->anum; ++j) if(strncasecmp((char *)AVI->idx[i], AVI->track[j].audio_tag,4) == 0) nai[j]++;
   }

   AVI->video_frames = nvi;
   for(j=0; j<AVI->anum; ++j) AVI->track[j].audio_chunks = nai[j];

   if(AVI->video_frames==0) ERR_EXIT(AVI_ERR_NO_VIDS);
   AVI->video_index = plat_malloc(nvi*sizeof(video_index_entry));
   if(AVI->video_index==0) ERR_EXIT(AVI_ERR_NO_MEM);

   for(j=0; j<AVI->anum; ++j) {
       if(AVI->track[j].audio_chunks) {
           AVI->track[j].audio_index = plat_zalloc((nai[j]+1)*sizeof(audio_index_entry));
           if(AVI->track[j].audio_index==0) ERR_EXIT(AVI_ERR_NO_MEM);
       }
   }

   nvi = 0;
   for(j=0; j<AVI->anum; ++j) nai[j] = tot[j] = 0;

   ioff = idx_type == 1 ? 8 : AVI->movi_start+4;

   for(i=0;i<AVI->n_idx;i++) {

     //video
     if(strncasecmp((char *)AVI->idx[i],AVI->video_tag,3) == 0) {
       AVI->video_index[nvi].key = str2ulong(AVI->idx[i]+ 4);
       AVI->video_index[nvi].pos = str2ulong(AVI->idx[i]+ 8)+ioff;
       AVI->video_index[nvi].len = str2ulong(AVI->idx[i]+12);
       AVI->v_streamsize += AVI->video_index[nvi].len; /* Frank Sinapsi - 2009-12-22 */
       nvi++;
     }

     //audio
     for(j=0; j<AVI->anum; ++j) {

       if(strncasecmp((char *)AVI->idx[i],AVI->track[j].audio_tag,4) == 0) {
         AVI->track[j].audio_index[nai[j]].pos = str2ulong(AVI->idx[i]+ 8)+ioff;
         AVI->track[j].audio_index[nai[j]].len = str2ulong(AVI->idx[i]+12);
         AVI->track[j].audio_index[nai[j]].tot = tot[j];
         tot[j] += AVI->track[j].audio_index[nai[j]].len;
         nai[j]++;
       }
     }
   }

   for(j=0; j<AVI->anum; ++j) AVI->track[j].audio_bytes = tot[j];

   } // is no opendml

   /* Reposition the file */

   fseeko(AVI->fdes,AVI->movi_start,SEEK_SET);
   AVI->video_pos = 0;

   return 0;
}

long AVI_video_frames(avi_t *AVI)
{
   return AVI->video_frames;
}
int  AVI_video_width(avi_t *AVI)
{
   return AVI->width;
}
int  AVI_video_height(avi_t *AVI)
{
   return AVI->height;
}
double AVI_frame_rate(avi_t *AVI)
{
   return AVI->fps;
}
char* AVI_video_compressor(avi_t *AVI)
{
   return AVI->compressor2;
}

long AVI_max_video_chunk(avi_t *AVI)
{
   return AVI->max_len;
}

int AVI_audio_tracks(avi_t *AVI)
{
    return(AVI->anum);
}

int AVI_audio_channels(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_chans;
}

long AVI_audio_mp3rate(avi_t *AVI)
{
   return AVI->track[AVI->aptr].mp3rate;
}

long AVI_audio_padrate(avi_t *AVI)
{
   return AVI->track[AVI->aptr].padrate;
}

int AVI_audio_bits(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_bits;
}

int AVI_audio_format(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_fmt;
}

long AVI_audio_rate(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_rate;
}

long AVI_audio_bytes(avi_t *AVI)
{
   return AVI->track[AVI->aptr].audio_bytes;
}

long AVI_audio_chunks(avi_t *AVI)
{
   return AVI->track[AVI->aptr].audio_chunks;
}

long AVI_audio_codech_offset(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_codech_off;
}

long AVI_audio_codecf_offset(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_codecf_off;
}

long  AVI_video_codech_offset(avi_t *AVI)
{
    return AVI->v_codech_off;
}

long  AVI_video_codecf_offset(avi_t *AVI)
{
    return AVI->v_codecf_off;
}

/*
 added - Frank Sinapsi - 2009-12-22
 */
long AVI_keyframes(avi_t *AVI, uns32b *min, uns32b *max)
{
    long frame, res = 0;
    uns32b lmin = 0xffffffff, lmax = 1, cnt = 1;

    if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
    if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

    for ( frame = 0 ; frame < AVI->video_frames ; frame++ )
        if ( AVI->video_index[frame].key == 0x10 ) {
            res++;
            if ( frame && ( cnt < lmin ) )
                lmin = cnt;
            cnt = 1;
        } else {
            cnt++;
            if ( cnt > lmax )
                lmax = cnt;
        }
    if ( lmin > lmax )
        lmin = lmax;
    if ( min )
        *min = lmin;
    if ( max )
        *max = lmax;
    return res;
}

long AVI_frame_size(avi_t *AVI, long frame)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(frame < 0 || frame >= AVI->video_frames) return 0;
   return(AVI->video_index[frame].len);
}

long AVI_audio_size(avi_t *AVI, long frame)
{
  if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
  if(!AVI->track[AVI->aptr].audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

  if(frame < 0 || frame >= AVI->track[AVI->aptr].audio_chunks) return -1;
  return(AVI->track[AVI->aptr].audio_index[frame].len);
}

long AVI_get_video_position(avi_t *AVI, long frame)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(frame < 0 || frame >= AVI->video_frames) return 0;
   return(AVI->video_index[frame].pos);
}

int AVI_seek_start(avi_t *AVI)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }

   fseeko(AVI->fdes,AVI->movi_start,SEEK_SET);
   AVI->video_pos = 0;
   return 0;
}

int AVI_set_video_position(avi_t *AVI, long frame)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if (frame < 0 ) frame = 0;
   AVI->video_pos = frame;
   return 0;
}

#if 0

long AVI_read_video(avi_t *AVI, char *vidbuf, long bytes, int *keyframe)
{
   long n;

   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(AVI->video_pos < 0 || AVI->video_pos >= AVI->video_frames) return -1;
   n = AVI->video_index[AVI->video_pos].len;

   if (bytes != -1 && bytes < n) {
     AVI_errno = AVI_ERR_NO_BUFSIZE;
     return -1;
   }

   *keyframe = (AVI->video_index[AVI->video_pos].key==0x10) ? 1:0;

   if (vidbuf == NULL) {
     AVI->video_pos++;
     return n;
   }

   fseeko(AVI->fdes, AVI->video_index[AVI->video_pos].pos, SEEK_SET);

   if (plat_read(AVI->fdes,vidbuf,n) != n)
   {
      AVI_errno = AVI_ERR_READ;
      return -1;
   }

   AVI->video_pos++;

   return n;
}

long AVI_read_frame(avi_t *AVI, char *vidbuf, int *keyframe)
{
   return AVI_read_video(AVI, vidbuf, -1, keyframe);
}

#endif

long AVI_read_video(avi_t *AVI, char **vidbuf, long *bytes, int *keyframe, long frame)
{
    if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
    if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }
    if(frame < 0 || frame >= AVI->video_frames) return -1;
    if ( *vidbuf ) {
        if ( *bytes < AVI->video_index[frame].len ) {
            AVI_errno = AVI_ERR_NO_BUFSIZE;
            return -1;
        }
        *bytes = AVI->video_index[frame].len;
    } else {
        *bytes = AVI->video_index[frame].len;
        if ( ( *vidbuf = GC_malloc_atomic( *bytes ) ) == NULL ) {
            AVI_errno = AVI_ERR_NO_BUFSIZE;
            return -1;
        }
    }

    *keyframe = (AVI->video_index[frame].key==0x10) ? 1:0;

    fseeko(AVI->fdes, AVI->video_index[frame].pos, SEEK_SET);

    if (plat_read(AVI->fdes,*vidbuf,*bytes) != *bytes)
    {
        AVI_errno = AVI_ERR_READ;
        return -1;
    }
    return *bytes;
}

long AVI_get_audio_position_index(avi_t *AVI)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->track[AVI->aptr].audio_index) { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   return (AVI->track[AVI->aptr].audio_posc);
}

int AVI_set_audio_position_index(avi_t *AVI, long indexpos)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->track[AVI->aptr].audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }
   if(indexpos > AVI->track[AVI->aptr].audio_chunks)     { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   AVI->track[AVI->aptr].audio_posc = indexpos;
   AVI->track[AVI->aptr].audio_posb = 0;

   return 0;
}

int AVI_set_audio_position(avi_t *AVI, long byte)
{
   long n0, n1, n;

   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->track[AVI->aptr].audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(byte < 0) byte = 0;

   /* Binary search in the audio chunks */

   n0 = 0;
   n1 = AVI->track[AVI->aptr].audio_chunks;

   while(n0<n1-1)
   {
      n = (n0+n1)/2;
      if(AVI->track[AVI->aptr].audio_index[n].tot>byte)
         n1 = n;
      else
         n0 = n;
   }

   AVI->track[AVI->aptr].audio_posc = n0;
   AVI->track[AVI->aptr].audio_posb = byte - AVI->track[AVI->aptr].audio_index[n0].tot;

   return 0;
}

long AVI_read_audio(avi_t *AVI, char *audbuf, long bytes)
{
   long nr, left, todo;
   trp_off_t pos;

   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->track[AVI->aptr].audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   nr = 0; /* total number of bytes read */

   if (bytes==0) {
     AVI->track[AVI->aptr].audio_posc++;
     AVI->track[AVI->aptr].audio_posb = 0;
     fseeko(AVI->fdes, 0LL, SEEK_CUR);
   }
   while(bytes>0)
   {
       trp_off_t ret;
      left = AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].len - AVI->track[AVI->aptr].audio_posb;
      if(left==0)
      {
         if(AVI->track[AVI->aptr].audio_posc>=AVI->track[AVI->aptr].audio_chunks-1) return nr;
         AVI->track[AVI->aptr].audio_posc++;
         AVI->track[AVI->aptr].audio_posb = 0;
         continue;
      }
      if(bytes<left)
         todo = bytes;
      else
         todo = left;
      pos = AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].pos + AVI->track[AVI->aptr].audio_posb;
      fseeko(AVI->fdes, pos, SEEK_SET);
      if ( (ret = plat_read(AVI->fdes,audbuf+nr,todo)) != todo)
      {
            plat_log_send(PLAT_LOG_DEBUG, __FILE__, "XXX pos = %lld, ret = %lld, todo = %ld",
                     (long long)pos, (long long)ret, todo);
         AVI_errno = AVI_ERR_READ;
         return -1;
      }
      bytes -= todo;
      nr    += todo;
      AVI->track[AVI->aptr].audio_posb += todo;
   }

   return nr;
}

long AVI_read_audio_chunk(avi_t *AVI, char *audbuf)
{
   long left;
   trp_off_t pos;

   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->track[AVI->aptr].audio_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if (AVI->track[AVI->aptr].audio_posc+1>AVI->track[AVI->aptr].audio_chunks) return -1;

   left = AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].len - AVI->track[AVI->aptr].audio_posb;

   if (audbuf == NULL) return left;

   if(left==0) {
       AVI->track[AVI->aptr].audio_posc++;
       AVI->track[AVI->aptr].audio_posb = 0;
       return 0;
   }

   pos = AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].pos + AVI->track[AVI->aptr].audio_posb;
   fseeko(AVI->fdes, pos, SEEK_SET);
   if (plat_read(AVI->fdes,audbuf,left) != left)
   {
      AVI_errno = AVI_ERR_READ;
      return -1;
   }
   AVI->track[AVI->aptr].audio_posc++;
   AVI->track[AVI->aptr].audio_posb = 0;

   return left;
}

/* AVI_print_error: Print most recent error (similar to perror) */

static const char *avi_errors[] =
{
  /*  0 */ "avilib - No Error",
  /*  1 */ "avilib - AVI file size limit reached",
  /*  2 */ "avilib - Error opening AVI file",
  /*  3 */ "avilib - Error reading from AVI file",
  /*  4 */ "avilib - Error writing to AVI file",
  /*  5 */ "avilib - Error writing index (file may still be useable)",
  /*  6 */ "avilib - Error closing AVI file",
  /*  7 */ "avilib - Operation (read/write) not permitted",
  /*  8 */ "avilib - Out of memory (malloc failed)",
  /*  9 */ "avilib - Not an AVI file",
  /* 10 */ "avilib - AVI file has no header list (corrupted?)",
  /* 11 */ "avilib - AVI file has no MOVI list (corrupted?)",
  /* 12 */ "avilib - AVI file has no video data",
  /* 13 */ "avilib - operation needs an index",
  /* 14 */ "avilib - destination buffer is too small",
  /* 15 */ "avilib - Unkown Error"
};
static int num_avi_errors = sizeof(avi_errors)/sizeof(char*);

void AVI_print_error(const char *str)
{
    int aerrno = (AVI_errno>=0 && AVI_errno<num_avi_errors) ?AVI_errno :num_avi_errors-1;

    if (aerrno != 0)
        plat_log_send(PLAT_LOG_ERROR, __FILE__, "%s: %s", str, avi_errors[aerrno]);

    /* for the following errors, perror should report a more detailed reason: */
    if (AVI_errno == AVI_ERR_OPEN
     || AVI_errno == AVI_ERR_READ
     || AVI_errno == AVI_ERR_WRITE
     || AVI_errno == AVI_ERR_WRITE_INDEX
     || AVI_errno == AVI_ERR_CLOSE) {
        perror("REASON");
    }
}

const char *AVI_strerror(void)
{
    static char error_string[4096];
    int aerrno = (AVI_errno>=0 && AVI_errno<num_avi_errors) ?AVI_errno :num_avi_errors-1;

    if (AVI_errno == AVI_ERR_OPEN
     || AVI_errno == AVI_ERR_READ
     || AVI_errno == AVI_ERR_WRITE
     || AVI_errno == AVI_ERR_WRITE_INDEX
     || AVI_errno == AVI_ERR_CLOSE ) {
        snprintf(error_string, sizeof(error_string), "%s - %s",avi_errors[aerrno],strerror(errno));
        return error_string;
    }
    return avi_errors[aerrno];
}

uns64b AVI_max_size(void)
{
    return((uns64b)AVI_MAX_LEN);
}


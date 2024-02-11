/*
 *  avilib.c
 *
 *  Copyright (C) Thomas Oestreich - June 2001
 *  multiple audio track support Copyright (C) 2002 Thomas Oestreich
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

/*
#define PACKAGE "transcode"
#define VERSION "1.0.0"
*/

#include "../trp/trp.h"
#include "avilib.h"

#ifdef MINGW
#define xio_lseek lseek64
#define TRP_RDONLY _O_RDONLY
#else
#define xio_lseek lseek
#define TRP_RDONLY O_RDONLY
#endif

// Maximum number of indices per stream
#define NR_IXNN_CHUNKS 32

#undef DEBUG_ODML

static void *avi_malloc( size_t size )
{
    void *p;

    if ( p = malloc( size ) )
        memset( p, 0, size );
    return p;
}

static ssize_t avi_read(int fd, char *buf, size_t len)
{
   ssize_t n, r = 0;

   while ( r < len ) {
      n = read( fd, buf + r, len - r );
      if ( n == 0 )
          break;
      if ( n < 0 ) {
          if ( errno == EINTR )
              continue;
          else
              break;
      }
      r += n;
   }
   return r;
}

#define PAD_EVEN(x) ( ((x)+1) & ~1 )

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

static uint64_t str2ullong(unsigned char *str)
{
   uint64_t r = (str[0] | (str[1]<<8) | (str[2]<<16) | (str[3]<<24));
   uint64_t s = (str[4] | (str[5]<<8) | (str[6]<<16) | (str[7]<<24));
   return ((s<<32)&0xffffffff00000000ULL)|(r&0xffffffffULL);
}

static uint32_t str2ulong(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) | (str[2]<<16) | (str[3]<<24) );
}
static uint32_t str2ushort(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) );
}

// bit 31 denotes a keyframe
static uint32_t str2ulong_len (unsigned char *str)
{
   return str2ulong(str) & 0x7fffffff;
}

// if bit 31 is 0, its a keyframe
static uint32_t str2ulong_key (unsigned char *str)
{
  uint32_t c = str2ulong(str);
  return ((c & 0x80000000) ? 0 : 0x10);
}

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

// #undef NR_IXNN_CHUNKS

static int avi_add_index_entry(avi_t *AVI, const unsigned char *tag, long flags,
                               unsigned long pos, unsigned long len)
{
   void *ptr;

   if(AVI->n_idx>=AVI->max_idx) {
     ptr = realloc((void *)AVI->idx,(AVI->max_idx+4096)*16);

     if(ptr == 0)
       return -1;
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

/* Returns 1 if more audio is in that video junk */
int AVI_can_read_audio(avi_t *AVI)
{
   if(!AVI->video_index)         { return -1; }
   if(!AVI->track[AVI->aptr].audio_index)         { return -1; }

   // is it -1? the last ones got left out --tibit
   //if (AVI->track[AVI->aptr].audio_posc>=AVI->track[AVI->aptr].audio_chunks-1) {
   if (AVI->track[AVI->aptr].audio_posc>=AVI->track[AVI->aptr].audio_chunks) {
       return 0;
   }

   if (AVI->video_pos >= AVI->video_frames) return 1;

   if (AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].pos < AVI->video_index[AVI->video_pos].pos) return 1;
   else return 0;
}

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
// see ../docs/avi_comments.txt
// returns the length of written stream (-1 on error)
static int avi_parse_comments (int fd, char *buf, int space_left)
{
    int len=0, readlen=0, k;
    char *data, *c, *d;
    struct stat st;

    // safety checks
    if (fd<=0 || !buf || space_left<=0)
        return -1;

    memset (buf, 0, space_left);
    if (fstat (fd, &st) == -1) {
        perror ("stat");
        return -1;
    }

    if ( !(data = avi_malloc(st.st_size*sizeof(char)+1)) ) {
        fprintf(stderr, "malloc failed\n");
        return -1;
    }

    readlen = avi_read ( fd, data, st.st_size);

    //printf("Read %d bytes from %d\n", readlen, fd);

    c = data;
    space_left--;

    while (len < space_left) {
        if ( !c || *c == '\0')
            break; // eof;
        else if (*c == '#') { // comment, ignore
            c = strchr(c, '\n')+1;
        }
        else if (*c == '\n') { // empty, ignore
            c++;
        }
        else if (*c == 'I') {

            // do not write ISFT -- this is written by transcode
            // and important for debugging.
            if (!valid_info_tag(c)) {
                // skip this line
                while (c && *c && *c != '\n' ) {
                    c++;
                }
                continue;
            }

            // set d after TAG
            d = c+4;

            // find first non-blank (!space or !TAB)
            while ( d && *d && (*d == ' ' || *d == '	')) ++d;
            if (!d) break;

            // TAG without argument is fine but ignored
            if (*d == '\n' || *d == '\r') {
                c = d+1;
                if (*c == '\n') ++c;
                continue;
            }

            k = 0;
            while (d[k] != '\r' && d[k] != '\n' && d[k] != '\0') ++k;
            if (k>=space_left) return len;

            // write TAG
            memcpy(buf+len,c,4);
            len += 4;

            // write length + '\0'
            long2str(buf+len, k+1); len += 4;

            // write comment string
            memcpy (buf+len, d, k);
            // must be null terminated
            *(buf+len+k+1) = '\0';

            // PAD
            if ((k+1)&1) {
                k++;
                *(buf+len+k+1) = '\0';
            }
            len += k+1;

            // advance c
            while (*c != '\n' && *c != '\0') ++c;
            if (*c != '\0') ++c;
            else break;

        } else {

            // ignore junk lines
            while (c && *c && *c != '\n' ) {
                if (*c == ' ' || *c == '	') { c++; break; }
                c++;
            }
            if (!c) break;
        }

    }
    free(data);

    return len;
}

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

void AVI_set_audio_vbr(avi_t *AVI, long is_vbr)
{
    AVI->track[AVI->aptr].a_vbr = is_vbr;
}

long AVI_get_audio_vbr(avi_t *AVI)
{
    return(AVI->track[AVI->aptr].a_vbr);
}

void AVI_set_comment_fd(avi_t *AVI, int fd)
{
    AVI->comment_fd = fd;
}
int  AVI_get_comment_fd(avi_t *AVI)
{
    return AVI->comment_fd;
}

/*******************************************************************
 *                                                                 *
 *    Utilities for reading video and audio from an AVI File       *
 *                                                                 *
 *******************************************************************/

static int AVI_close_low(uns8b to_free,avi_t *AVI)
{
   int j,k;

   /* Even if there happened an error, we first clean up */

   if (AVI->comment_fd>0)
       close(AVI->comment_fd);
   AVI->comment_fd = -1;
   close(AVI->fdes);
   free(AVI->idx);
   free(AVI->video_index);
   if(AVI->video_superindex) {
       if (AVI->video_superindex->stdindex) {
           for (j = 0; j < NR_IXNN_CHUNKS; j++)
               if (AVI->video_superindex->stdindex[j]) {
                   free(AVI->video_superindex->stdindex[j]->aIndex);
                   free(AVI->video_superindex->stdindex[j]);
               }
           free(AVI->video_superindex->stdindex);
       }
       free(AVI->video_superindex->aIndex);
       free(AVI->video_superindex);
   }

   for (j=0; j<AVI->anum; j++)
   {
       avisuperindex_chunk *a = AVI->track[j].audio_superindex;

       free(AVI->wave_format_ex[j]);
       free(AVI->track[j].audio_index);
       if( a ) {
           if (a->stdindex) {
               for (k = 0; k < NR_IXNN_CHUNKS; k++)
                   if (a->stdindex[k]) {
                       free(a->stdindex[k]->aIndex);
                       free(a->stdindex[k]);
                   }
               free(a->stdindex);
           }
           free(a->aIndex);
           free(a);
       }
   }

   free(AVI->bitmap_info_header);
   if ( to_free )
      free(AVI);

   return 0;
}

int AVI_close(avi_t *AVI)
{
   return AVI_close_low(1,AVI);
}

#define ERR_EXIT \
{ \
   AVI_close_low(0,AVI); \
   AVI->avierrno = 1; \
   return; \
}

static void avi_parse_input_file(avi_t *AVI, int getIndex)
{
  long i, rate, scale, idx_type;
  off_t n;
  unsigned char *hdrl_data;
  long header_offset=0, hdrl_len=0;
  long nvi, nai[AVI_MAX_TRACKS], ioff;
  long tot[AVI_MAX_TRACKS];
  int j;
  int lasttag = 0;
  int vids_strh_seen = 0;
  int vids_strf_seen = 0;
  int auds_strh_seen = 0;
  //  int auds_strf_seen = 0;
  int num_stream = 0;
  char data[256];
  off_t oldpos=-1, newpos=-1;

  /* Read first 12 bytes and check that this is an AVI file */

   if( avi_read(AVI->fdes,data,12) != 12 ) ERR_EXIT

   if( strncasecmp(data  ,"RIFF",4) !=0 ||
       strncasecmp(data+8,"AVI ",4) !=0 ) ERR_EXIT

   /* Go through the AVI file and extract the header list,
      the start position of the 'movi' list and an optionally
      present idx1 tag */

   hdrl_data = 0;

   while(1)
   {
      if( avi_read(AVI->fdes,data,8) != 8 ) break; /* We assume it's EOF */
      newpos=xio_lseek(AVI->fdes,0,SEEK_CUR);
      if(oldpos==newpos) {
              /* This is a broken AVI stream... */
              ERR_EXIT
      }
      oldpos=newpos;

      n = str2ulong((unsigned char *)data+4);
      n = PAD_EVEN(n);

      if(strncasecmp(data,"LIST",4) == 0)
      {
         if( avi_read(AVI->fdes,data,4) != 4 ) ERR_EXIT
         n -= 4;
         if(strncasecmp(data,"hdrl",4) == 0)
         {
            hdrl_len = n;
            hdrl_data = (unsigned char *) avi_malloc(n);
            if(hdrl_data==0) ERR_EXIT

            // offset of header

            header_offset = xio_lseek(AVI->fdes,0,SEEK_CUR);

            if( avi_read(AVI->fdes,(char *)hdrl_data,n) != n ) ERR_EXIT
         }
         else if(strncasecmp(data,"movi",4) == 0)
         {
            AVI->movi_start = xio_lseek(AVI->fdes,0,SEEK_CUR);
            if (xio_lseek(AVI->fdes,n,SEEK_CUR)==(off_t)-1) break;
         }
         else
            if (xio_lseek(AVI->fdes,n,SEEK_CUR)==(off_t)-1) break;
      }
      else if(strncasecmp(data,"idx1",4) == 0)
      {
         /* n must be a multiple of 16, but the reading does not
            break if this is not the case */

         AVI->n_idx = AVI->max_idx = n/16;
         AVI->idx = (unsigned  char((*)[16]) ) avi_malloc(n);
         if(AVI->idx==0) ERR_EXIT
         if(avi_read(AVI->fdes, (char *) AVI->idx, n) != n ) {
             free ( AVI->idx); AVI->idx=NULL;
             AVI->n_idx = 0;
         }
      }
      else
         xio_lseek(AVI->fdes,n,SEEK_CUR);
   }

   if(!hdrl_data      ) ERR_EXIT
   if(!AVI->movi_start) ERR_EXIT

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
            AVI->v_scale = scale; /* FS 2006-09-08 */
            AVI->v_rate = rate; /* FS 2006-09-08 */
            AVI->v_start = str2ulong(hdrl_data+i+28); /* FS 2006-09-08 */
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
#ifdef FS_DEBUG
               fprintf(stderr, "error - only %d audio tracks supported\n", AVI_MAX_TRACKS);
#endif
             ERR_EXIT
           }

           AVI->track[AVI->aptr].audio_bytes = str2ulong(hdrl_data+i+32)*avi_sampsize(AVI, 0);
           AVI->track[AVI->aptr].audio_strn = num_stream;

           // if samplesize==0 -> vbr
           AVI->track[AVI->aptr].a_vbr = !str2ulong(hdrl_data+i+44);

           AVI->track[AVI->aptr].a_scale = str2ulong(hdrl_data+i+20); /* FS 2006-09-08 */
           AVI->track[AVI->aptr].padrate = str2ulong(hdrl_data+i+24);
           AVI->track[AVI->aptr].a_start = str2ulong(hdrl_data+i+28); /* FS 2006-09-08 */

           //	   auds_strh_seen = 1;
           lasttag = 2; /* auds */

           // ThOe
           AVI->track[AVI->aptr].a_codech_off = header_offset + i;

         }
         else if (strncasecmp (hdrl_data+i,"iavs",4) ==0 && ! auds_strh_seen) {
#ifdef FS_DEBUG
             fprintf(stderr, "AVILIB: error - DV AVI Type 1 no supported\n");
#endif
             ERR_EXIT
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
            AVI->bitmap_info_header = (alBITMAPINFOHEADER *)
                avi_malloc(str2ulong((unsigned char *)&bih.bi_size));
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
            wfe = (alWAVEFORMATEX *)avi_malloc(sizeof(alWAVEFORMATEX));
            if (wfe != NULL) {
              memcpy(wfe, hdrl_data + i, wfes);
              if (str2ushort((unsigned char *)&wfe->cb_size) != 0) {
                nwfe = (char *)
                  realloc(wfe, sizeof(alWAVEFORMATEX) +
                          str2ushort((unsigned char *)&wfe->cb_size));
                if (nwfe != 0) {
                  off_t lpos = xio_lseek(AVI->fdes, 0, SEEK_CUR);
                  xio_lseek(AVI->fdes, header_offset + i + sizeof(alWAVEFORMATEX),
                        SEEK_SET);
                  wfe = (alWAVEFORMATEX *)nwfe;
                  nwfe = &nwfe[sizeof(alWAVEFORMATEX)];
                  avi_read(AVI->fdes, nwfe,
                           str2ushort((unsigned char *)&wfe->cb_size));
                  xio_lseek(AVI->fdes, lpos, SEEK_SET);
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

            AVI->video_superindex = (avisuperindex_chunk *) avi_malloc (sizeof (avisuperindex_chunk));
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
#ifdef FS_DEBUG
                fprintf(stderr, "Invalid Header, bIndexSubType != 0\n");
#endif
            }

            AVI->video_superindex->aIndex =
               avi_malloc (AVI->video_superindex->wLongsPerEntry * AVI->video_superindex->nEntriesInUse * sizeof (uint32_t));

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

            AVI->track[AVI->aptr].audio_superindex = (avisuperindex_chunk *) avi_malloc (sizeof (avisuperindex_chunk));
            memcpy (AVI->track[AVI->aptr].audio_superindex->fcc, a, 4);             a += 4;
            AVI->track[AVI->aptr].audio_superindex->dwSize = str2ulong(a);          a += 4;
            AVI->track[AVI->aptr].audio_superindex->wLongsPerEntry = str2ushort(a); a += 2;
            AVI->track[AVI->aptr].audio_superindex->bIndexSubType = *a;             a += 1;
            AVI->track[AVI->aptr].audio_superindex->bIndexType = *a;                a += 1;
            AVI->track[AVI->aptr].audio_superindex->nEntriesInUse = str2ulong(a);   a += 4;
            memcpy (AVI->track[AVI->aptr].audio_superindex->dwChunkId, a, 4);       a += 4;

            // 3 * reserved
            a += 4; a += 4; a += 4;

            if (AVI->track[AVI->aptr].audio_superindex->bIndexSubType != 0) {
#ifdef FS_DEBUG
                fprintf(stderr, "Invalid Header, bIndexSubType != 0\n");
#endif
            }

            AVI->track[AVI->aptr].audio_superindex->aIndex =
               avi_malloc (AVI->track[AVI->aptr].audio_superindex->wLongsPerEntry *
                     AVI->track[AVI->aptr].audio_superindex->nEntriesInUse * sizeof (uint32_t));

            // position of ix## chunks
            for (j=0; j<AVI->track[AVI->aptr].audio_superindex->nEntriesInUse; ++j) {
               AVI->track[AVI->aptr].audio_superindex->aIndex[j].qwOffset = str2ullong (a);  a += 8;
               AVI->track[AVI->aptr].audio_superindex->aIndex[j].dwSize = str2ulong (a);     a += 4;
               AVI->track[AVI->aptr].audio_superindex->aIndex[j].dwDuration = str2ulong (a); a += 4;

#ifdef DEBUG_ODML
               printf("[%d] 0x%llx 0x%lx %lu\n", j,
                       (unsigned long long)AVI->track[AVI->aptr].audio_superindex->aIndex[j].qwOffset,
                       (unsigned long)AVI->track[AVI->aptr].audio_superindex->aIndex[j].dwSize,
                       (unsigned long)AVI->track[AVI->aptr].audio_superindex->aIndex[j].dwDuration);
#endif
            }

#ifdef DEBUG_ODML
            printf("FOURCC \"%.4s\"\n", AVI->track[AVI->aptr].audio_superindex->fcc);
            printf("LEN \"%ld\"\n", (long)AVI->track[AVI->aptr].audio_superindex->dwSize);
            printf("wLongsPerEntry \"%d\"\n", AVI->track[AVI->aptr].audio_superindex->wLongsPerEntry);
            printf("bIndexSubType \"%d\"\n", AVI->track[AVI->aptr].audio_superindex->bIndexSubType);
            printf("bIndexType \"%d\"\n", AVI->track[AVI->aptr].audio_superindex->bIndexType);
            printf("nEntriesInUse \"%ld\"\n", (long)AVI->track[AVI->aptr].audio_superindex->nEntriesInUse);
//	    printf("dwChunkId \"%.4s\"\n", AVI->track[AVI->aptr].audio_superindex->dwChunkId[0]);
            printf("--\n");
#endif

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

   free(hdrl_data);

   if(!vids_strh_seen || !vids_strf_seen) ERR_EXIT

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

   xio_lseek(AVI->fdes,AVI->movi_start,SEEK_SET);

   if(!getIndex) return;

   /* if the file has an idx1, check if this is relative
      to the start of the file or to the start of the movi list */

   idx_type = 0;

   if(AVI->idx)
   {
      off_t pos, len;

      /* Search the first videoframe in the idx1 and look where
         it is in the file */

      for(i=0;i<AVI->n_idx;i++)
         if( strncasecmp((char *)AVI->idx[i],(char *)AVI->video_tag,3)==0 ) break;
      if(i>=AVI->n_idx) ERR_EXIT

      pos = str2ulong(AVI->idx[i]+ 8);
      len = str2ulong(AVI->idx[i]+12);

      xio_lseek(AVI->fdes,pos,SEEK_SET);
      if(avi_read(AVI->fdes,data,8)!=8) ERR_EXIT
      if( strncasecmp(data,(char *)AVI->idx[i],4)==0 && str2ulong((unsigned char *)data+4)==len )
      {
         idx_type = 1; /* Index from start of file */
      }
      else
      {
         xio_lseek(AVI->fdes,pos+AVI->movi_start-4,SEEK_SET);
         if(avi_read(AVI->fdes,data,8)!=8) ERR_EXIT
         if( strncasecmp(data,(char *)AVI->idx[i],4)==0 && str2ulong((unsigned char *)data+4)==len )
         {
            idx_type = 2; /* Index from start of movi list */
         }
      }
      /* idx_type remains 0 if neither of the two tests above succeeds */
   }

   if(idx_type == 0 && !AVI->is_opendml && !AVI->total_frames)
   {
      /* we must search through the file to get the index */

      xio_lseek(AVI->fdes, AVI->movi_start, SEEK_SET);

      AVI->n_idx = 0;

      while(1)
      {
         if( avi_read(AVI->fdes,data,8) != 8 ) break;
         n = str2ulong((unsigned char *)data+4);

         /* The movi list may contain sub-lists, ignore them */

         if(strncasecmp(data,"LIST",4)==0)
         {
            xio_lseek(AVI->fdes,4,SEEK_CUR);
            continue;
         }

         /* Check if we got a tag ##db, ##dc or ##wb */

         if( ( (data[2]=='d' || data[2]=='D') &&
               (data[3]=='b' || data[3]=='B' || data[3]=='c' || data[3]=='C') )
             || ( (data[2]=='w' || data[2]=='W') &&
                  (data[3]=='b' || data[3]=='B') ) )
           {
           if ( avi_add_index_entry(AVI,(unsigned char *)data,0,xio_lseek(AVI->fdes,0,SEEK_CUR)-8,n) )
              ERR_EXIT
         }

         xio_lseek(AVI->fdes,PAD_EVEN(n),SEEK_CUR);
      }
      idx_type = 1;
   }

   // ************************
   // OPENDML
   // ************************

   // read extended index chunks
   if (AVI->is_opendml) {
      uint64_t offset = 0;
      int hdrl_len = 4+4+2+1+1+4+4+8+4;
      char *en, *chunk_start;
      int k = 0, audtr = 0;
      uint32_t nrEntries = 0;

      AVI->video_index = NULL;

      nvi = 0;
      for(audtr=0; audtr<AVI->anum; ++audtr) nai[audtr] = tot[audtr] = 0;

      // ************************
      // VIDEO
      // ************************

      for (j=0; j<AVI->video_superindex->nEntriesInUse; j++) {

         // read from file
         chunk_start = en = avi_malloc (AVI->video_superindex->aIndex[j].dwSize+hdrl_len);

         if (xio_lseek(AVI->fdes, AVI->video_superindex->aIndex[j].qwOffset, SEEK_SET) == (off_t)-1) {
#ifdef FS_DEBUG
            fprintf(stderr, "(%s) cannot seek to 0x%llx\n", __FILE__,
                    (unsigned long long)AVI->video_superindex->aIndex[j].qwOffset);
#endif
            free(chunk_start);
            continue;
         }

         if (avi_read(AVI->fdes, en, AVI->video_superindex->aIndex[j].dwSize+hdrl_len) <= 0) {
#ifdef FS_DEBUG
            fprintf(stderr, "(%s) cannot read from offset 0x%llx %ld bytes; broken (incomplete) file?\n",
                  __FILE__, (unsigned long long)AVI->video_superindex->aIndex[j].qwOffset,
                  (unsigned long)AVI->video_superindex->aIndex[j].dwSize+hdrl_len);
#endif
            free(chunk_start);
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
         AVI->video_index = (video_index_entry *) realloc (AVI->video_index, nvi * sizeof (video_index_entry));
         if (!AVI->video_index) {
                 fprintf(stderr, "AVILIB: out of mem (size = %ld)\n", nvi * sizeof (video_index_entry));
                 exit(1);
         }

         while (k < nvi) {

            AVI->video_index[k].pos = offset + str2ulong(en); en += 4;
            AVI->video_index[k].len = str2ulong_len(en);
            AVI->v_streamsize += AVI->video_index[k].len; /* FS */
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

         free(chunk_start);
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
#ifdef FS_DEBUG
             fprintf(stderr, "(%s) cannot read audio index for track %d\n", __FILE__, audtr);
#endif
             continue;
         }
         for (j=0; j<AVI->track[audtr].audio_superindex->nEntriesInUse; j++) {

            // read from file
            chunk_start = en = avi_malloc (AVI->track[audtr].audio_superindex->aIndex[j].dwSize+hdrl_len);

            if (xio_lseek(AVI->fdes, AVI->track[audtr].audio_superindex->aIndex[j].qwOffset, SEEK_SET) == (off_t)-1) {
#ifdef FS_DEBUG
               fprintf(stderr, "(%s) cannot seek to 0x%llx\n", __FILE__, (unsigned long long)AVI->track[audtr].audio_superindex->aIndex[j].qwOffset);
#endif
               free(chunk_start);
               continue;
            }

            if (avi_read(AVI->fdes, en, AVI->track[audtr].audio_superindex->aIndex[j].dwSize+hdrl_len) <= 0) {
#ifdef FS_DEBUG
               fprintf(stderr, "(%s) cannot read from offset 0x%llx; broken (incomplete) file?\n",
                     __FILE__,(unsigned long long) AVI->track[audtr].audio_superindex->aIndex[j].qwOffset);
#endif
               free(chunk_start);
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
            AVI->track[audtr].audio_index = (audio_index_entry *) realloc (AVI->track[audtr].audio_index, nai[audtr] * sizeof (audio_index_entry));

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

            free(chunk_start);
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
multiple_riff:

      xio_lseek(AVI->fdes, AVI->movi_start, SEEK_SET);

      AVI->n_idx = 0;

#ifdef FS_DEBUG
      fprintf(stderr, "[avilib] Reconstructing index...");
#endif

      // Number of frames; only one audio track supported
      nvi = AVI->video_frames = AVI->total_frames;
      nai[0] = AVI->track[0].audio_chunks = AVI->total_frames;
      for(j=1; j<AVI->anum; ++j) AVI->track[j].audio_chunks = 0;

      AVI->video_index = (video_index_entry *) avi_malloc(nvi*sizeof(video_index_entry));

      if(AVI->video_index==0) ERR_EXIT

      for(j=0; j<AVI->anum; ++j) {
          if(AVI->track[j].audio_chunks) {
              AVI->track[j].audio_index = (audio_index_entry *) avi_malloc((nai[j]+1)*sizeof(audio_index_entry));
              if(AVI->track[j].audio_index==0) ERR_EXIT
          }
      }

      nvi = 0;
      for(j=0; j<AVI->anum; ++j) nai[j] = tot[j] = 0;

      aud_chunks = AVI->total_frames;

      while(1)
      {
         if (nvi >= AVI->total_frames) break;

         if( avi_read(AVI->fdes,data,8) != 8 ) break;
         n = str2ulong((unsigned char *)data+4);

         j=0;

         if (aud_chunks - nai[j] -1 <= 0) {
             aud_chunks += AVI->total_frames;
             AVI->track[j].audio_index = (audio_index_entry *)
                 realloc( AVI->track[j].audio_index, (aud_chunks+1)*sizeof(audio_index_entry));
             if (!AVI->track[j].audio_index) {
#ifdef FS_DEBUG
                 fprintf(stderr, "Internal error in avilib -- no mem\n");
#endif
                 ERR_EXIT
             }
         }

         /* Check if we got a tag ##db, ##dc or ##wb */

         // VIDEO
         if(
             (data[0]=='0' || data[1]=='0') &&
             (data[2]=='d' || data[2]=='D') &&
             (data[3]=='b' || data[3]=='B' || data[3]=='c' || data[3]=='C') ) {

             AVI->video_index[nvi].key = 0x0;
             AVI->video_index[nvi].pos = xio_lseek(AVI->fdes,0,SEEK_CUR);
             AVI->video_index[nvi].len = n;
             AVI->v_streamsize += n; /* FS */

             /*
             fprintf(stderr, "Frame %ld pos %lld len %lld key %ld\n",
                     nvi, AVI->video_index[nvi].pos,  AVI->video_index[nvi].len, (long)AVI->video_index[nvi].key);
                     */
             nvi++;
             xio_lseek(AVI->fdes,PAD_EVEN(n),SEEK_CUR);
         }

         //AUDIO
         else if(
                 (data[0]=='0' || data[1]=='1') &&
                 (data[2]=='w' || data[2]=='W') &&
             (data[3]=='b' || data[3]=='B') ) {

                AVI->track[j].audio_index[nai[j]].pos = xio_lseek(AVI->fdes,0,SEEK_CUR);
                AVI->track[j].audio_index[nai[j]].len = n;
                AVI->track[j].audio_index[nai[j]].tot = tot[j];
                tot[j] += AVI->track[j].audio_index[nai[j]].len;
                nai[j]++;

                xio_lseek(AVI->fdes,PAD_EVEN(n),SEEK_CUR);
         }
         else {
            xio_lseek(AVI->fdes,-4,SEEK_CUR);
         }

      }
      if (nvi < AVI->total_frames) {
#ifdef FS_DEBUG
          fprintf(stderr, "\n[avilib] Uh? Some frames seems missing (%ld/%d)\n",
                  nvi,  AVI->total_frames);
#endif
      }

      AVI->video_frames = nvi;
      AVI->track[0].audio_chunks = nai[0];

      for(j=0; j<AVI->anum; ++j) AVI->track[j].audio_bytes = tot[j];
      idx_type = 1;
#ifdef FS_DEBUG
      fprintf(stderr, "done. nvi=%ld nai=%ld tot=%ld\n", nvi, nai[0], tot[0]);
#endif
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

   if(AVI->video_frames==0) ERR_EXIT
   AVI->video_index = (video_index_entry *) avi_malloc(nvi*sizeof(video_index_entry));
   if(AVI->video_index==0) ERR_EXIT

   for(j=0; j<AVI->anum; ++j) {
       if(AVI->track[j].audio_chunks) {
           AVI->track[j].audio_index = (audio_index_entry *) avi_malloc((nai[j]+1)*sizeof(audio_index_entry));
           if(AVI->track[j].audio_index==0) ERR_EXIT
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
       AVI->v_streamsize += AVI->video_index[nvi].len; /* FS */
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

   xio_lseek(AVI->fdes,AVI->movi_start,SEEK_SET);
   AVI->video_pos = 0;
}

avi_t *AVI_open_input_file(const char *filename, int getIndex)
{
  avi_t *AVI;

  /* Create avi_t structure */

  AVI = (avi_t *) avi_malloc(sizeof(avi_t));
  if ( AVI == NULL )
     return NULL;

  /* Open the file */

  AVI->fdes = trp_open(filename,TRP_RDONLY);
  if ( AVI->fdes < 0 ) {
      free( AVI );
      return NULL;
  }
  AVI->avierrno = 0;
  avi_parse_input_file( AVI, getIndex );
  if ( AVI->avierrno ) {
      free( AVI );
      return NULL;
  }
  AVI->aptr=0; //reset
  return AVI;
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

/*
 FS -- inizio
 */

long long AVI_video_streamsize(avi_t *AVI)
{
    return AVI->v_streamsize;
}

long AVI_video_scale(avi_t *AVI)
{
    return AVI->v_scale;
}

long AVI_video_rate(avi_t *AVI)
{
    return AVI->v_rate;
}

long AVI_video_start(avi_t *AVI)
{
    return AVI->v_start;
}

/*
 FS -- fine
 */

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

long AVI_audio_scale(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_scale;
}

long AVI_audio_start(avi_t *AVI)
{
   return AVI->track[AVI->aptr].a_start;
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

long AVI_frame_size(avi_t *AVI, long frame)
{
   if(!AVI->video_index) return -1;

   if(frame < 0 || frame >= AVI->video_frames) return 0;
   return(AVI->video_index[frame].len);
}

long AVI_keyframes(avi_t *AVI, unsigned long *min, unsigned long *max)
/*
 FS -- aggiunta da me
 */
{
   long frame, res = 0;
   unsigned long lmin = 0xffffffff, lmax = 1, cnt = 1;

   if(!AVI->video_index) return -1;

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

long AVI_audio_size(avi_t *AVI, long frame)
{
  if(!AVI->track[AVI->aptr].audio_index) return -1;

  if(frame < 0 || frame >= AVI->track[AVI->aptr].audio_chunks) return -1;
  return(AVI->track[AVI->aptr].audio_index[frame].len);
}

long AVI_get_video_position(avi_t *AVI, long frame)
{
   if(!AVI->video_index) return -1;

   if(frame < 0 || frame >= AVI->video_frames) return 0;
   return(AVI->video_index[frame].pos);
}

int AVI_seek_start(avi_t *AVI)
{
   xio_lseek(AVI->fdes,AVI->movi_start,SEEK_SET);
   AVI->video_pos = 0;
   return 0;
}

int AVI_set_video_position(avi_t *AVI, long frame)
{
   if(!AVI->video_index) return -1;

   if (frame < 0 ) frame = 0;
   AVI->video_pos = frame;
   return 0;
}

long AVI_read_frame(avi_t *AVI, char *vidbuf, int *keyframe)
{
   long n;

   if(!AVI->video_index) return -1;

   if(AVI->video_pos < 0 || AVI->video_pos >= AVI->video_frames) return -1;
   n = AVI->video_index[AVI->video_pos].len;

   *keyframe = (AVI->video_index[AVI->video_pos].key==0x10) ? 1:0;

   if (vidbuf == NULL) {
     AVI->video_pos++;
     return n;
   }

   xio_lseek(AVI->fdes, AVI->video_index[AVI->video_pos].pos, SEEK_SET);

   if (avi_read(AVI->fdes,vidbuf,n) != n)
      return -1;

   AVI->video_pos++;

   return n;
}

long AVI_get_audio_position_index(avi_t *AVI)
{
   if(!AVI->track[AVI->aptr].audio_index) return -1;

   return (AVI->track[AVI->aptr].audio_posc);
}

int AVI_set_audio_position_index(avi_t *AVI, long indexpos)
{
   if(!AVI->track[AVI->aptr].audio_index) return -1;
   if(indexpos > AVI->track[AVI->aptr].audio_chunks) return -1;

   AVI->track[AVI->aptr].audio_posc = indexpos;
   AVI->track[AVI->aptr].audio_posb = 0;

   return 0;
}

int AVI_set_audio_position(avi_t *AVI, long byte)
{
   long n0, n1, n;

   if(!AVI->track[AVI->aptr].audio_index) return -1;

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
   off_t pos;

   if(!AVI->track[AVI->aptr].audio_index) return -1;

   nr = 0; /* total number of bytes read */

   if (bytes==0) {
     AVI->track[AVI->aptr].audio_posc++;
     AVI->track[AVI->aptr].audio_posb = 0;
      xio_lseek(AVI->fdes, 0LL, SEEK_CUR);
   }
   while(bytes>0)
   {
       off_t ret;
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
      xio_lseek(AVI->fdes, pos, SEEK_SET);
      if ( (ret = avi_read(AVI->fdes,audbuf+nr,todo)) != todo)
      {
#ifdef FS_DEBUG
         fprintf(stderr, "XXX pos = %lld, ret = %lld, todo = %ld\n",
                     (long long)pos, (long long)ret, todo);
#endif
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
   off_t pos;

   if(!AVI->track[AVI->aptr].audio_index) return -1;

   if (AVI->track[AVI->aptr].audio_posc+1>AVI->track[AVI->aptr].audio_chunks) return -1;

   left = AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].len - AVI->track[AVI->aptr].audio_posb;

   if (audbuf == NULL) return left;

   if(left==0) {
       AVI->track[AVI->aptr].audio_posc++;
       AVI->track[AVI->aptr].audio_posb = 0;
       return 0;
   }

   pos = AVI->track[AVI->aptr].audio_index[AVI->track[AVI->aptr].audio_posc].pos + AVI->track[AVI->aptr].audio_posb;
   xio_lseek(AVI->fdes, pos, SEEK_SET);
   if (avi_read(AVI->fdes,audbuf,left) != left)
      return -1;
   AVI->track[AVI->aptr].audio_posc++;
   AVI->track[AVI->aptr].audio_posb = 0;

   return left;
}

/* AVI_read_data: Special routine for reading the next audio or video chunk
                  without having an index of the file. */

int AVI_read_data(avi_t *AVI, char *vidbuf, long max_vidbuf,
                              char *audbuf, long max_audbuf,
                              long *len)
{

/*
 * Return codes:
 *
 *    1 = video data read
 *    2 = audio data read
 *    0 = reached EOF
 *   -1 = video buffer too small
 *   -2 = audio buffer too small
 */

   off_t n;
   char data[8];

   while(1)
   {
      /* Read tag and length */

      if( avi_read(AVI->fdes,data,8) != 8 ) return 0;

      /* if we got a list tag, ignore it */

      if(strncasecmp(data,"LIST",4) == 0)
      {
         xio_lseek(AVI->fdes,4,SEEK_CUR);
         continue;
      }

      n = PAD_EVEN(str2ulong((unsigned char *)data+4));

      if(strncasecmp(data,AVI->video_tag,3) == 0)
      {
         *len = n;
         AVI->video_pos++;
         if(n>max_vidbuf)
         {
            xio_lseek(AVI->fdes,n,SEEK_CUR);
            return -1;
         }
         if(avi_read(AVI->fdes,vidbuf,n) != n ) return 0;
         return 1;
      }
      else if(strncasecmp(data,AVI->track[AVI->aptr].audio_tag,4) == 0)
      {
         *len = n;
         if(n>max_audbuf)
         {
            xio_lseek(AVI->fdes,n,SEEK_CUR);
            return -2;
         }
         if(avi_read(AVI->fdes,audbuf,n) != n ) return 0;
         return 2;
         break;
      }
      else
         if(xio_lseek(AVI->fdes,n,SEEK_CUR)<0)  return 0;
   }
}


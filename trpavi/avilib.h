/*
 *  avilib.h
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

#ifndef AVILIB_H
#define AVILIB_H

#include "../trp/trp.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef MINGW
#define trp_off_t sig64b
#else
#define trp_off_t off_t
#endif

#define AVI_MAX_TRACKS 8

enum {
  AVI_ERROR = -1,
  AVI_OK = 0,
};

typedef struct
{
  trp_off_t key;
  trp_off_t pos;
  trp_off_t len;
} video_index_entry;

typedef struct
{
   trp_off_t pos;
   trp_off_t len;
   trp_off_t tot;
} audio_index_entry;

// Index types

#define AVI_INDEX_OF_INDEXES 0x00             // when each entry in aIndex
                                              // array points to an index chunk
#define AVI_INDEX_OF_CHUNKS  0x01             // when each entry in aIndex
                                              // array points to a chunk in the file
#define AVI_INDEX_IS_DATA    0x80             // when each entry is aIndex is
                                              // really the data
// bIndexSubtype codes for INDEX_OF_CHUNKS
//
#define AVI_INDEX_2FIELD     0x01             // when fields within frames
                                              // are also indexed

typedef struct _avisuperindex_entry {
    uns64b qwOffset;           // absolute file offset
    uns32b dwSize;                  // size of index chunk at this offset
    uns32b dwDuration;              // time span in stream ticks
} avisuperindex_entry;

typedef struct _avistdindex_entry {
    uns32b dwOffset;                // qwBaseOffset + this is absolute file offset
    uns32b dwSize;                  // bit 31 is set if this is NOT a keyframe
} avistdindex_entry;

// Standard index
typedef struct _avistdindex_chunk {
    char           fcc[4];                 // ix##
    uns32b  dwSize;                 // size of this chunk
    uns16b wLongsPerEntry;         // must be sizeof(aIndex[0])/sizeof(DWORD)
    uns8b  bIndexSubType;          // must be 0
    uns8b  bIndexType;             // must be AVI_INDEX_OF_CHUNKS
    uns32b  nEntriesInUse;          //
    char           dwChunkId[4];           // '##dc' or '##db' or '##wb' etc..
    uns64b qwBaseOffset;       // all dwOffsets in aIndex array are relative to this
    uns32b  dwReserved3;            // must be 0
    avistdindex_entry *aIndex;
} avistdindex_chunk;

// Base Index Form 'indx'
typedef struct _avisuperindex_chunk {
    char           fcc[4];
    uns32b  dwSize;                 // size of this chunk
    uns16b wLongsPerEntry;         // size of each entry in aIndex array (must be 8 for us)
    uns8b  bIndexSubType;          // future use. must be 0
    uns8b  bIndexType;             // one of AVI_INDEX_* codes
    uns32b  nEntriesInUse;          // index of first unused member in aIndex array
    char           dwChunkId[4];           // fcc of what is indexed
    uns32b  dwReserved[3];          // meaning differs for each index type/subtype.
                                           // 0 if unused
    avisuperindex_entry *aIndex;           // where are the ix## chunks
    avistdindex_chunk **stdindex;          // the ix## chunks itself (array)
} avisuperindex_chunk;

typedef struct track_s
{
    long   a_fmt;             /* Audio format, see #defines below */
    long   a_chans;           /* Audio channels, 0 for no audio */
    long   a_rate;            /* Rate in Hz */
    long   a_bits;            /* bits per audio sample */
    long   mp3rate;           /* mp3 bitrate kbs*/
    long   a_vbr;             /* 0 == no Variable BitRate */
    long   padrate;       /* byte rate used for zero padding */

    long   audio_strn;        /* Audio stream number */
    trp_off_t  audio_bytes;       /* Total number of bytes of audio data */
    long   audio_chunks;      /* Chunks of audio data in the file */

    char   audio_tag[4];      /* Tag of audio data */
    long   audio_posc;        /* Audio position: chunk */
    long   audio_posb;        /* Audio position: byte within chunk */

    long   a_scale; /* Frank Sinapsi - 2009-12-22 */
    long   a_start; /* Frank Sinapsi - 2009-12-22 */

    trp_off_t  a_codech_off;       /* absolut offset of audio codec information */
    trp_off_t  a_codecf_off;       /* absolut offset of audio codec information */

    audio_index_entry *audio_index;
    avisuperindex_chunk *audio_superindex;

} track_t;

typedef struct
{
  uns32b  bi_size;
  uns32b  bi_width;
  uns32b  bi_height;
  uns16b  bi_planes;
  uns16b  bi_bit_count;
  uns32b  bi_compression;
  uns32b  bi_size_image;
  uns32b  bi_x_pels_per_meter;
  uns32b  bi_y_pels_per_meter;
  uns32b  bi_clr_used;
  uns32b  bi_clr_important;
} alBITMAPINFOHEADER;

typedef struct __attribute__((__packed__))
{
  uns16b  w_format_tag;
  uns16b  n_channels;
  uns32b  n_samples_per_sec;
  uns32b  n_avg_bytes_per_sec;
  uns16b  n_block_align;
  uns16b  w_bits_per_sample;
  uns16b  cb_size;
} alWAVEFORMATEX;

typedef struct __attribute__((__packed__))
{
  uns32b fcc_type;
  uns32b fcc_handler;
  uns32b dw_flags;
  uns32b dw_caps;
  uns16b w_priority;
  uns16b w_language;
  uns32b dw_scale;
  uns32b dw_rate;
  uns32b dw_start;
  uns32b dw_length;
  uns32b dw_initial_frames;
  uns32b dw_suggested_buffer_size;
  uns32b dw_quality;
  uns32b dw_sample_size;
  uns32b dw_left;
  uns32b dw_top;
  uns32b dw_right;
  uns32b dw_bottom;
  uns32b dw_edit_count;
  uns32b dw_format_change_count;
  char     sz_name[64];
} alAVISTREAMINFO;

typedef struct
{
  FILE   *fdes;             /* File descriptor of AVI file */
  long   mode;              /* 0 for reading, 1 for writing */

  long   width;             /* Width  of a video frame */
  long   height;            /* Height of a video frame */
  double fps;               /* Frames per second */
  char   compressor[8];     /* Type of compressor, 4 bytes + padding for 0 byte */
  char   compressor2[8];     /* Type of compressor, 4 bytes + padding for 0 byte */
  long   video_strn;        /* Video stream number */
  long   video_frames;      /* Number of video frames */
  char   video_tag[4];      /* Tag of video data */
  long   video_pos;         /* Number of next frame to be read
                   (if index present) */

  uns32b max_len;    /* maximum video chunk present */

  track_t track[AVI_MAX_TRACKS];  // up to AVI_MAX_TRACKS audio tracks supported

  trp_off_t  pos;               /* position in file */
  long   n_idx;             /* number of index entries actually filled */
  long   max_idx;           /* number of index entries actually allocated */

  trp_off_t  v_codech_off;      /* absolut offset of video codec (strh) info */
  trp_off_t  v_codecf_off;      /* absolut offset of video codec (strf) info */

  uns8b (*idx)[16]; /* index entries (AVI idx1 tag) */

  video_index_entry *video_index;
  avisuperindex_chunk *video_superindex;  /* index of indices */
  int is_opendml;           /* set to 1 if this is an odml file with multiple index chunks */

  trp_off_t  last_pos;          /* Position of last frame written */
  uns32b last_len;   /* Length of last frame written */
  int must_use_index;       /* Flag if frames are duplicated */
  trp_off_t  movi_start;
  int total_frames;         /* total number of frames if dmlh is present */

  sig64b v_streamsize; /* Frank Sinapsi - 2009-12-22 */
  long   v_scale;      /* Frank Sinapsi - 2009-12-22 */
  long   v_rate;       /* Frank Sinapsi - 2009-12-22 */
  long   v_start;      /* Frank Sinapsi - 2009-12-22 */

  int anum;            // total number of audio tracks
  int aptr;            // current audio working track
  char *index_file;    // read the avi index from this file

  alBITMAPINFOHEADER *bitmap_info_header;
  alWAVEFORMATEX *wave_format_ex[AVI_MAX_TRACKS];

  void*     extradata;
  unsigned long extradata_size;
} avi_t;

#define AVI_MODE_WRITE  0
#define AVI_MODE_READ   1

/* The error codes delivered by avi_open_input_file */

#define AVI_ERR_SIZELIM      1     /* The write of the data would exceed
                                      the maximum size of the AVI file.
                                      This is more a warning than an error
                                      since the file may be closed safely */

#define AVI_ERR_OPEN         2     /* Error opening the AVI file - wrong path
                                      name or file nor readable/writable */

#define AVI_ERR_READ         3     /* Error reading from AVI File */

#define AVI_ERR_WRITE        4     /* Error writing to AVI File,
                                      disk full ??? */

#define AVI_ERR_WRITE_INDEX  5     /* Could not write index to AVI file
                                      during close, file may still be
                                      usable */

#define AVI_ERR_CLOSE        6     /* Could not write header to AVI file
                                      or not truncate the file during close,
                                      file is most probably corrupted */

#define AVI_ERR_NOT_PERM     7     /* Operation not permitted:
                                      trying to read from a file open
                                      for writing or vice versa */

#define AVI_ERR_NO_MEM       8     /* malloc failed */

#define AVI_ERR_NO_AVI       9     /* Not an AVI file */

#define AVI_ERR_NO_HDRL     10     /* AVI file has no has no header list,
                                      corrupted ??? */

#define AVI_ERR_NO_MOVI     11     /* AVI file has no has no MOVI list,
                                      corrupted ??? */

#define AVI_ERR_NO_VIDS     12     /* AVI file contains no video data */

#define AVI_ERR_NO_IDX      13     /* The file has been opened with
                                      getIndex==0, but an operation has been
                                      performed that needs an index */
#define AVI_ERR_NO_BUFSIZE  14     /* Given buffer is not large enough
                                      to hold the requested data */

/* Possible Audio formats */

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_UNKNOWN             (0x0000)
#define WAVE_FORMAT_PCM                 (0x0001)
#define WAVE_FORMAT_ADPCM               (0x0002)
#define WAVE_FORMAT_IBM_CVSD            (0x0005)
#define WAVE_FORMAT_ALAW                (0x0006)
#define WAVE_FORMAT_MULAW               (0x0007)
#define WAVE_FORMAT_OKI_ADPCM           (0x0010)
#define WAVE_FORMAT_DVI_ADPCM           (0x0011)
#define WAVE_FORMAT_DIGISTD             (0x0015)
#define WAVE_FORMAT_DIGIFIX             (0x0016)
#define WAVE_FORMAT_YAMAHA_ADPCM        (0x0020)
#define WAVE_FORMAT_DSP_TRUESPEECH      (0x0022)
#define WAVE_FORMAT_GSM610              (0x0031)
#define IBM_FORMAT_MULAW                (0x0101)
#define IBM_FORMAT_ALAW                 (0x0102)
#define IBM_FORMAT_ADPCM                (0x0103)
#endif

int  AVI_close(avi_t *AVI);

avi_t *AVI_open_input_file(const char *filename, int getIndex);

long AVI_audio_mp3rate(avi_t *AVI);
long AVI_audio_padrate(avi_t *AVI);
long AVI_video_frames(avi_t *AVI);
int  AVI_video_width(avi_t *AVI);
int  AVI_video_height(avi_t *AVI);
double AVI_frame_rate(avi_t *AVI);
char* AVI_video_compressor(avi_t *AVI);

int  AVI_audio_channels(avi_t *AVI);
int  AVI_audio_bits(avi_t *AVI);
int  AVI_audio_format(avi_t *AVI);
long AVI_audio_rate(avi_t *AVI);
long AVI_audio_bytes(avi_t *AVI);
long AVI_audio_chunks(avi_t *AVI);

long AVI_max_video_chunk(avi_t *AVI);

long AVI_keyframes(avi_t *AVI, uns32b *min, uns32b *max); /* Frank Sinapsi - 2009-12-22 */
long AVI_frame_size(avi_t *AVI, long frame);
long AVI_audio_size(avi_t *AVI, long frame);
int  AVI_seek_start(avi_t *AVI);
int  AVI_set_video_position(avi_t *AVI, long frame);
long AVI_get_video_position(avi_t *AVI, long frame);
#if 0
long AVI_read_video(avi_t *AVI, char *vidbuf, long bytes, int *keyframe);
long AVI_read_frame(avi_t *AVI, char *vidbuf, int *keyframe);
#endif
long AVI_read_video(avi_t *AVI, char **vidbuf, long *bytes, int *keyframe, long frame);

int  AVI_set_audio_position(avi_t *AVI, long byte);

long AVI_get_audio_position_index(avi_t *AVI);
int  AVI_set_audio_position_index(avi_t *AVI, long indexpos);

long AVI_read_audio(avi_t *AVI, char *audbuf, long bytes);
long AVI_read_audio_chunk(avi_t *AVI, char *audbuf);

long AVI_audio_codech_offset(avi_t *AVI);
long AVI_audio_codecf_offset(avi_t *AVI);
long AVI_video_codech_offset(avi_t *AVI);
long AVI_video_codecf_offset(avi_t *AVI);

void AVI_print_error(const char *str);
const char *AVI_strerror(void);

int AVI_scan(const char *name);
int AVI_dump(const char *name, int mode);

uns64b AVI_max_size(void);

int AVI_set_audio_track(avi_t *AVI, int track);
int AVI_get_audio_track(avi_t *AVI);
int AVI_audio_tracks(avi_t *AVI);

long AVI_get_audio_vbr(avi_t *AVI);

struct riff_struct
{
    uns8b id[4];   /* RIFF */
    uns32b len;
    uns8b wave_id[4]; /* WAVE */
};

struct chunk_struct
{
    uns8b id[4];
    uns32b len;
};

struct common_struct
{
    uns16b wFormatTag;
    uns16b wChannels;
    uns32b dwSamplesPerSec;
    uns32b dwAvgBytesPerSec;
    uns16b wBlockAlign;
    uns16b wBitsPerSample;  /* Only for PCM */
};

struct wave_header
{
    struct riff_struct   riff;
    struct chunk_struct  format;
    struct common_struct common;
    struct chunk_struct  data;
};

struct AVIStreamHeader {
    uns32b  fccType;
    uns32b  fccHandler;
    uns32b  dwFlags;
    uns32b  dwPriority;
    uns32b  dwInitialFrames;
    uns32b  dwScale;
    uns32b  dwRate;
    uns32b  dwStart;
    uns32b  dwLength;
    uns32b  dwSuggestedBufferSize;
    uns32b  dwQuality;
    uns32b  dwSampleSize;
} __attribute__((__packed__));

#endif /* AVILIB_H */

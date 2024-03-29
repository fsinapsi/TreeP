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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>

#include <unistd.h>
#include <stdint.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define AVI_MAX_TRACKS 8

typedef struct
{
  off_t key;
  off_t pos;
  off_t len;
} video_index_entry;

typedef struct
{
   off_t pos;
   off_t len;
   off_t tot;
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
    uint64_t qwOffset;           // absolute file offset
    uint32_t dwSize;                  // size of index chunk at this offset
    uint32_t dwDuration;              // time span in stream ticks
} avisuperindex_entry;

typedef struct _avistdindex_entry {
    uint32_t dwOffset;                // qwBaseOffset + this is absolute file offset
    uint32_t dwSize;                  // bit 31 is set if this is NOT a keyframe
} avistdindex_entry;

// Standard index
typedef struct _avistdindex_chunk {
    char           fcc[4];                 // ix##
    uint32_t  dwSize;                 // size of this chunk
    uint16_t wLongsPerEntry;         // must be sizeof(aIndex[0])/sizeof(DWORD)
    uint8_t  bIndexSubType;          // must be 0
    uint8_t  bIndexType;             // must be AVI_INDEX_OF_CHUNKS
    uint32_t  nEntriesInUse;          //
    char           dwChunkId[4];           // '##dc' or '##db' or '##wb' etc..
    uint64_t qwBaseOffset;       // all dwOffsets in aIndex array are relative to this
    uint32_t  dwReserved3;            // must be 0
    avistdindex_entry *aIndex;
} avistdindex_chunk;

// Base Index Form 'indx'
typedef struct _avisuperindex_chunk {
    char           fcc[4];
    uint32_t  dwSize;                 // size of this chunk
    uint16_t wLongsPerEntry;         // size of each entry in aIndex array (must be 8 for us)
    uint8_t  bIndexSubType;          // future use. must be 0
    uint8_t  bIndexType;             // one of AVI_INDEX_* codes
    uint32_t  nEntriesInUse;          // index of first unused member in aIndex array
    char           dwChunkId[4];           // fcc of what is indexed
    uint32_t  dwReserved[3];          // meaning differs for each index type/subtype.
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
    long   padrate;	      /* byte rate used for zero padding */

    long   audio_strn;        /* Audio stream number */
    off_t  audio_bytes;       /* Total number of bytes of audio data */
    long   audio_chunks;      /* Chunks of audio data in the file */

    char   audio_tag[4];      /* Tag of audio data */
    long   audio_posc;        /* Audio position: chunk */
    long   audio_posb;        /* Audio position: byte within chunk */

    long   a_scale; /* FS 2006-09-08 */
    long   a_start; /* FS 2006-09-08 */

    off_t  a_codech_off;       /* absolut offset of audio codec information */
    off_t  a_codecf_off;       /* absolut offset of audio codec information */

    audio_index_entry *audio_index;
    avisuperindex_chunk *audio_superindex;

} track_t;

typedef struct
{
  uint32_t  bi_size;
  uint32_t  bi_width;
  uint32_t  bi_height;
  uint16_t  bi_planes;
  uint16_t  bi_bit_count;
  uint32_t  bi_compression;
  uint32_t  bi_size_image;
  uint32_t  bi_x_pels_per_meter;
  uint32_t  bi_y_pels_per_meter;
  uint32_t  bi_clr_used;
  uint32_t  bi_clr_important;
} alBITMAPINFOHEADER;

typedef struct __attribute__((__packed__))
{
  uint16_t  w_format_tag;
  uint16_t  n_channels;
  uint32_t  n_samples_per_sec;
  uint32_t  n_avg_bytes_per_sec;
  uint16_t  n_block_align;
  uint16_t  w_bits_per_sample;
  uint16_t  cb_size;
} alWAVEFORMATEX;

typedef struct __attribute__((__packed__))
{
  uint32_t fcc_type;
  uint32_t fcc_handler;
  uint32_t dw_flags;
  uint32_t dw_caps;
  uint16_t w_priority;
  uint16_t w_language;
  uint32_t dw_scale;
  uint32_t dw_rate;
  uint32_t dw_start;
  uint32_t dw_length;
  uint32_t dw_initial_frames;
  uint32_t dw_suggested_buffer_size;
  uint32_t dw_quality;
  uint32_t dw_sample_size;
  uint32_t dw_left;
  uint32_t dw_top;
  uint32_t dw_right;
  uint32_t dw_bottom;
  uint32_t dw_edit_count;
  uint32_t dw_format_change_count;
  char     sz_name[64];
} alAVISTREAMINFO;

typedef struct
{
  long   fdes;              /* File descriptor of AVI file */
  long   avierrno;

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

  uint32_t max_len;    /* maximum video chunk present */

  track_t track[AVI_MAX_TRACKS];  // up to AVI_MAX_TRACKS audio tracks supported

  off_t  pos;               /* position in file */
  long   n_idx;             /* number of index entries actually filled */
  long   max_idx;           /* number of index entries actually allocated */

  off_t  v_codech_off;      /* absolut offset of video codec (strh) info */
  off_t  v_codecf_off;      /* absolut offset of video codec (strf) info */

  uint8_t (*idx)[16]; /* index entries (AVI idx1 tag) */

  video_index_entry *video_index;
  avisuperindex_chunk *video_superindex;  /* index of indices */
  int is_opendml;           /* set to 1 if this is an odml file with multiple index chunks */

  off_t  last_pos;          /* Position of last frame written */
  uint32_t last_len;   /* Length of last frame written */
  int must_use_index;       /* Flag if frames are duplicated */
  off_t  movi_start;
  int total_frames;         /* total number of frames if dmlh is present */

  long long v_streamsize; /* FS 2006-10-08 */
  long   v_scale;   /* FS 2006-09-08 */
  long   v_rate;    /* FS 2006-09-08 */
  long   v_start;   /* FS 2006-09-08 */

  int anum;            // total number of audio tracks
  int aptr;            // current audio working track
  int comment_fd;      // Read avi header comments from this fd
  char *index_file;    // read the avi index from this file

  alBITMAPINFOHEADER *bitmap_info_header;
  alWAVEFORMATEX *wave_format_ex[AVI_MAX_TRACKS];

  void *extradata;
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

long long AVI_video_streamsize(avi_t *AVI);
long AVI_video_scale(avi_t *AVI);
long AVI_video_rate(avi_t *AVI);
long AVI_video_start(avi_t *AVI);

int  AVI_audio_channels(avi_t *AVI);
int  AVI_audio_bits(avi_t *AVI);
int  AVI_audio_format(avi_t *AVI);
long AVI_audio_rate(avi_t *AVI);
long AVI_audio_bytes(avi_t *AVI);

long AVI_audio_scale(avi_t *AVI);
long AVI_audio_start(avi_t *AVI);

long AVI_audio_chunks(avi_t *AVI);
int  AVI_can_read_audio(avi_t *AVI);

long AVI_max_video_chunk(avi_t *AVI);

long AVI_frame_size(avi_t *AVI, long frame);

long AVI_keyframes(avi_t *AVI, unsigned long *min, unsigned long *max);

long AVI_audio_size(avi_t *AVI, long frame);
int  AVI_seek_start(avi_t *AVI);
int  AVI_set_video_position(avi_t *AVI, long frame);
long AVI_get_video_position(avi_t *AVI, long frame);
long AVI_read_frame(avi_t *AVI, char *vidbuf, int *keyframe);

int  AVI_set_audio_position(avi_t *AVI, long byte);

long AVI_get_audio_position_index(avi_t *AVI);
int  AVI_set_audio_position_index(avi_t *AVI, long indexpos);

long AVI_read_audio(avi_t *AVI, char *audbuf, long bytes);
long AVI_read_audio_chunk(avi_t *AVI, char *audbuf);

long AVI_audio_codech_offset(avi_t *AVI);
long AVI_audio_codecf_offset(avi_t *AVI);
long AVI_video_codech_offset(avi_t *AVI);
long AVI_video_codecf_offset(avi_t *AVI);

int  AVI_read_data(avi_t *AVI, char *vidbuf, long max_vidbuf,
                               char *audbuf, long max_audbuf,
                               long *len);

int AVI_scan(const char *name);
int AVI_dump(const char *name, int mode);

int AVI_file_check(const char *import_file);

int avi_update_header(avi_t *AVI);

int AVI_set_audio_track(avi_t *AVI, int track);
int AVI_get_audio_track(avi_t *AVI);
int AVI_audio_tracks(avi_t *AVI);

void AVI_set_audio_vbr(avi_t *AVI, long is_vbr);
long AVI_get_audio_vbr(avi_t *AVI);

void AVI_set_comment_fd(avi_t *AVI, int fd);
int  AVI_get_comment_fd(avi_t *AVI);

struct riff_struct
{
  uint8_t id[4];   /* RIFF */
  uint32_t len;
  uint8_t wave_id[4]; /* WAVE */
};

struct chunk_struct
{
        uint8_t id[4];
        uint32_t len;
};

struct common_struct
{
        uint16_t wFormatTag;
        uint16_t wChannels;
        uint32_t dwSamplesPerSec;
        uint32_t dwAvgBytesPerSec;
        uint16_t wBlockAlign;
        uint16_t wBitsPerSample;  /* Only for PCM */
};

struct wave_header
{
        struct riff_struct   riff;
        struct chunk_struct  format;
        struct common_struct common;
        struct chunk_struct  data;
};

struct AVIStreamHeader {
  long  fccType;
  long  fccHandler;
  long  dwFlags;
  long  dwPriority;
  long  dwInitialFrames;
  long  dwScale;
  long  dwRate;
  long  dwStart;
  long  dwLength;
  long  dwSuggestedBufferSize;
  long  dwQuality;
  long  dwSampleSize;
};

#endif

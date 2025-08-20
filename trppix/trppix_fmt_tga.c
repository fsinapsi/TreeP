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
#include <stdbool.h>

#define TGA_MAX_IMAGE_DIMENSIONS 65535

#ifdef MINGW
#define fseeko fseeko64
#endif

///
/// \brief Image pixel format.
///
/// The pixel data are all in little-endian. E.g. a TGA_PIXEL_ARGB32 format
/// image, a single pixel is stored in the memory in the order of
/// BBBBBBBB GGGGGGGG RRRRRRRR AAAAAAAA.
///
enum tga_pixel_format {
    ///
    /// \brief Single channel format represents grayscale, 8-bit integer.
    ///
    TGA_PIXEL_BW8,
    ///
    /// \brief Single channel format represents grayscale, 16-bit integer.
    ///
    TGA_PIXEL_BW16,
    ///
    /// \brief A 16-bit pixel format.
    /// The topmost bit is assumed to an attribute bit, usually ignored.
    /// Because of little-endian, this format pixel is stored in the memory in
    /// the order of GGGBBBBB ARRRRRGG.
    ///
    TGA_PIXEL_RGB555,
    ///
    /// \brief RGB color format, 8-bit per channel.
    ///
    TGA_PIXEL_RGB24,
    ///
    /// \brief RGB color with alpha format, 8-bit per channel.
    ///
    TGA_PIXEL_ARGB32
};

enum tga_error {
    TGA_NO_ERROR = 0,
    TGA_ERROR_OUT_OF_MEMORY,
    TGA_ERROR_FILE_CANNOT_READ,
    TGA_ERROR_FILE_CANNOT_WRITE,
    TGA_ERROR_NO_DATA,
    TGA_ERROR_UNSUPPORTED_COLOR_MAP_TYPE,
    TGA_ERROR_UNSUPPORTED_IMAGE_TYPE,
    TGA_ERROR_UNSUPPORTED_PIXEL_FORMAT,
    TGA_ERROR_INVALID_IMAGE_DIMENSIONS,
    TGA_ERROR_COLOR_MAP_INDEX_FAILED
};

typedef struct {
    uns16b width;
    uns16b height;
    enum tga_pixel_format pixel_format;
    FILE *fp;
    bool has_read_file_error;
    uns8b *idata;
    uns32b isize;
    uns32b iread;
} tga_info;

static size_t trp_tga_fread( void *ptr, size_t nmemb, tga_info *info )
{
    if ( info->fp )
        nmemb = fread( ptr, 1, nmemb, info->fp );
    else {
        if ( nmemb > info->isize - info->iread )
            nmemb = info->isize - info->iread;
        memcpy( ptr, info->idata + info->iread, nmemb );
        info->iread += nmemb;
    }
    return nmemb;
}

static int trp_tga_fseek( tga_info *info, off_t offset )
{
    int res;

    if ( info->fp )
        res = fseeko( info->fp, offset, SEEK_CUR );
    else {
        off_t pos = info->iread + offset;

        if ( ( pos >= 0 ) && ( pos <= info->isize ) ) {
            info->iread = pos;
            res = 0;
        } else
            res = -1;
    }
    return res;
}

enum tga_error tga_create(uns8b **data_out, tga_info *info, int width,
                          int height, enum tga_pixel_format format);
void tga_image_flip_h( uns8b *data, tga_info *info );
void tga_image_flip_v( uns8b *data, tga_info *info );
static inline bool check_dimensions( int width, int height );
static inline int pixel_format_to_pixel_size( enum tga_pixel_format format );
static enum tga_error load_image( uns8b **data_out, tga_info *info );
static inline uns8b *get_pixel( uns8b *data, tga_info *info, int x, int y );
static enum tga_error save_image( uns8b *data, tga_info *info );

enum tga_error tga_create(uns8b **data_out, tga_info *info, int width,
                          int height, enum tga_pixel_format format)
{
    if (check_dimensions(width, height)) {
        return TGA_ERROR_INVALID_IMAGE_DIMENSIONS;
    }
    int pixel_size = pixel_format_to_pixel_size(format);
    if (pixel_size == -1) {
        return TGA_ERROR_UNSUPPORTED_PIXEL_FORMAT;
    }

    // Creates image data and info structure.
    uns8b *data = (uns8b *)calloc((size_t)width * height, pixel_size);
    if (data == NULL) {
        return TGA_ERROR_OUT_OF_MEMORY;
    }
    info->width = width;
    info->height = height;
    info->pixel_format = format;

    *data_out = data;
    return TGA_NO_ERROR;
}

void tga_image_flip_h(uns8b *data, tga_info *info) {
    if (data == NULL || info == NULL) {
        return;
    }
    int pixel_size = pixel_format_to_pixel_size(info->pixel_format);
    uns8b temp[pixel_size];
    int flip_num = info->width / 2;
    for (int i = 0; i < flip_num; ++i) {
        for (int j = 0; j < info->height; ++j) {
            uns8b *p1 = get_pixel(data, info, i, j);
            uns8b *p2 = get_pixel(data, info, info->width - 1 - i, j);
            // Swap two pixels.
            memcpy(temp, p1, pixel_size);
            memcpy(p1, p2, pixel_size);
            memcpy(p2, temp, pixel_size);
        }
    }
}

void tga_image_flip_v(uns8b *data, tga_info *info) {
    if (data == NULL || info == NULL) {
        return;
    }
    int pixel_size = pixel_format_to_pixel_size(info->pixel_format);
    uns8b temp[pixel_size];
    int flip_num = info->height / 2;
    for (int i = 0; i < flip_num; ++i) {
        for (int j = 0; j < info->width; ++j) {
            uns8b *p1 = get_pixel(data, info, j, i);
            uns8b *p2 = get_pixel(data, info, j, info->height - 1 - i);
            // Swap two pixels.
            memcpy(temp, p1, pixel_size);
            memcpy(p1, p2, pixel_size);
            memcpy(p2, temp, pixel_size);
        }
    }
}

enum tga_image_type {
    TGA_TYPE_NO_DATA = 0,
    TGA_TYPE_COLOR_MAPPED = 1,
    TGA_TYPE_TRUE_COLOR = 2,
    TGA_TYPE_GRAYSCALE = 3,
    TGA_TYPE_RLE_COLOR_MAPPED = 9,
    TGA_TYPE_RLE_TRUE_COLOR = 10,
    TGA_TYPE_RLE_GRAYSCALE = 11
};

struct tga_header {
    uns8b id_length;
    uns8b map_type;
    uns8b image_type;

    // Color map specification.
    uns16b map_first_entry;
    uns16b map_length;
    uns8b map_entry_size;

    // Image specification.
    uns16b image_x_origin;
    uns16b image_y_origin;
    uns16b image_width;
    uns16b image_height;
    uns8b pixel_depth;
    uns8b image_descriptor;
};

struct color_map {
    uns16b first_index;
    uns16b entry_count;
    uns8b bytes_per_entry;
    uns8b *pixels;
};

#define HEADER_SIZE 18

#define IS_SUPPORTED_IMAGE_TYPE(header)                  \
    ((header).image_type == TGA_TYPE_COLOR_MAPPED ||     \
     (header).image_type == TGA_TYPE_TRUE_COLOR ||       \
     (header).image_type == TGA_TYPE_GRAYSCALE ||        \
     (header).image_type == TGA_TYPE_RLE_COLOR_MAPPED || \
     (header).image_type == TGA_TYPE_RLE_TRUE_COLOR ||   \
     (header).image_type == TGA_TYPE_RLE_GRAYSCALE)

#define IS_COLOR_MAPPED(header)                      \
    ((header).image_type == TGA_TYPE_COLOR_MAPPED || \
     (header).image_type == TGA_TYPE_RLE_COLOR_MAPPED)

#define IS_TRUE_COLOR(header)                      \
    ((header).image_type == TGA_TYPE_TRUE_COLOR || \
     (header).image_type == TGA_TYPE_RLE_TRUE_COLOR)

#define IS_GRAYSCALE(header)                      \
    ((header).image_type == TGA_TYPE_GRAYSCALE || \
     (header).image_type == TGA_TYPE_RLE_GRAYSCALE)

#define IS_RLE(header)                                   \
    ((header).image_type == TGA_TYPE_RLE_COLOR_MAPPED || \
     (header).image_type == TGA_TYPE_RLE_TRUE_COLOR ||   \
     (header).image_type == TGA_TYPE_RLE_GRAYSCALE)

// Convert bits to integer bytes. E.g. 8 bits to 1 byte, 9 bits to 2 bytes.
#define BITS_TO_BYTES(bit_count) (((bit_count)-1) / 8 + 1)

// Reads a 8-bit integer from the file stream.
static inline uns8b read_uint8(tga_info *info) {
    uns8b value;
    if (trp_tga_fread(&value, 1, info) != 1) {
        info->has_read_file_error = true;
        return 0;
    }
    return value;
}

// Gets a 16-bit little-endian integer from the file stream.
// This function should works on both big-endian and little-endian architecture
// systems.
static inline uns16b read_uint16_le(tga_info *info) {
    uns8b buffer[2];
    if (trp_tga_fread(&buffer, 2, info) != 2) {
        info->has_read_file_error = true;
        return 0;
    }
    return buffer[0] + (((uns16b)buffer[1]) << 8);
}

// Checks if the picture size is correct.
// Returns true if invalid dimensisns, otherwise returns false.
static inline bool check_dimensions(int width, int height) {
    return width <= 0 || width > TGA_MAX_IMAGE_DIMENSIONS || height <= 0 ||
           height > TGA_MAX_IMAGE_DIMENSIONS;
}

// Gets the bytes per pixel by pixel format.
// Returns bytes per pxiel, otherwise returns -1 means the parameter `format` is
// invalid.
static inline int pixel_format_to_pixel_size(enum tga_pixel_format format) {
    switch (format) {
        case TGA_PIXEL_BW8:
            return 1;
        case TGA_PIXEL_BW16:
        case TGA_PIXEL_RGB555:
            return 2;
        case TGA_PIXEL_RGB24:
            return 3;
        case TGA_PIXEL_ARGB32:
            return 4;
        default:
            return -1;
    }
}

// Gets the pixel format according to the header.
// Returns false means the header is not illegal, otherwise returns true.
static bool get_pixel_format(enum tga_pixel_format *format,
                             struct tga_header *header) {
    if (IS_COLOR_MAPPED(*header)) {
        // If the supported pixel_depth is changed, remember to also change
        // the pixel_to_map_index() function.
        if (header->pixel_depth == 8) {
            switch (header->map_entry_size) {
                case 15:
                case 16:
                    *format = TGA_PIXEL_RGB555;
                    return false;
                case 24:
                    *format = TGA_PIXEL_RGB24;
                    return false;
                case 32:
                    *format = TGA_PIXEL_ARGB32;
                    return false;
            }
        }
    } else if (IS_TRUE_COLOR(*header)) {
        switch (header->pixel_depth) {
            case 16:
                *format = TGA_PIXEL_RGB555;
                return false;
            case 24:
                *format = TGA_PIXEL_RGB24;
                return false;
            case 32:
                *format = TGA_PIXEL_ARGB32;
                return false;
        }
    } else if (IS_GRAYSCALE(*header)) {
        switch (header->pixel_depth) {
            case 8:
                *format = TGA_PIXEL_BW8;
                return false;
            case 16:
                *format = TGA_PIXEL_BW16;
                return false;
        }
    }
    return true;
}

// Loads TGA header from file stream and returns the pixel format.
static enum tga_error load_header(struct tga_header *header,
                                  enum tga_pixel_format *pixel_format,
                                  tga_info *info) {
    info->has_read_file_error = false;

    header->id_length = read_uint8(info);
    header->map_type = read_uint8(info);
    header->image_type = read_uint8(info);
    header->map_first_entry = read_uint16_le(info);
    header->map_length = read_uint16_le(info);
    header->map_entry_size = read_uint8(info);
    header->image_x_origin = read_uint16_le(info);
    header->image_y_origin = read_uint16_le(info);
    header->image_width = read_uint16_le(info);
    header->image_height = read_uint16_le(info);
    header->pixel_depth = read_uint8(info);
    header->image_descriptor = read_uint8(info);

    if (info->has_read_file_error) {
        return TGA_ERROR_FILE_CANNOT_READ;
    }
    if (header->map_type > 1) {
        return TGA_ERROR_UNSUPPORTED_COLOR_MAP_TYPE;
    }
    if (header->image_type == TGA_TYPE_NO_DATA) {
        return TGA_ERROR_NO_DATA;
    }
    if (!IS_SUPPORTED_IMAGE_TYPE(*header)) {
        return TGA_ERROR_UNSUPPORTED_IMAGE_TYPE;
    }
    if (header->image_width <= 0 || header->image_height <= 0) {
        // No need to check if the image size exceeds TGA_MAX_IMAGE_DIMENSIONS.
        return TGA_ERROR_INVALID_IMAGE_DIMENSIONS;
    }
    if (get_pixel_format(pixel_format, header)) {
        return TGA_ERROR_UNSUPPORTED_PIXEL_FORMAT;
    }
    return TGA_NO_ERROR;
}

// Used for color mapped image decode.
static inline uns16b pixel_to_map_index(uns8b *pixel_ptr) {
    // Because only 8-bit index is supported now, so implemented in this way.
    return pixel_ptr[0];
}

// Gets the color of the specified index from the map.
// Returns false means no error, otherwise returns true.
static inline bool try_get_color_from_map(uns8b *dest, uns16b index,
                                          struct color_map *map) {
    index -= map->first_index;
    if (index < 0 && index >= map->entry_count) {
        return true;
    }
    memcpy(dest, map->pixels + map->bytes_per_entry * index,
           map->bytes_per_entry);
    return false;
}

// Decode image data from file stream.
static enum tga_error decode_data(uns8b *data, tga_info *info,
                                  uns8b pixel_size, bool is_color_mapped,
                                  struct color_map *map) {
    enum tga_error error_code = TGA_NO_ERROR;
    size_t pixel_count = (size_t)info->width * info->height;

    if (is_color_mapped) {
        for (; pixel_count > 0; --pixel_count) {
            if (trp_tga_fread(data, pixel_size, info) != pixel_size) {
                error_code = TGA_ERROR_FILE_CANNOT_READ;
                break;
            }
            // In color mapped image, the pixel as the index value of the color
            // map. The actual pixel value is found from the color map.
            uns16b index = pixel_to_map_index(data);
            if (try_get_color_from_map(data, index, map)) {
                error_code = TGA_ERROR_COLOR_MAP_INDEX_FAILED;
                break;
            }
            data += map->bytes_per_entry;
        }
    } else {
        size_t data_size = pixel_count * pixel_size;
        if (trp_tga_fread(data, data_size, info) != data_size) {
            error_code = TGA_ERROR_FILE_CANNOT_READ;
        }
    }
    return error_code;
}

// Decode image data with run-length encoding from file stream.
static enum tga_error decode_data_rle(uns8b *data, tga_info *info,
                                      uns8b pixel_size, bool is_color_mapped,
                                      struct color_map *map) {
    enum tga_error error_code = TGA_NO_ERROR;
    size_t pixel_count = (size_t)info->width * info->height;
    bool is_run_length_packet = false;
    uns8b packet_count = 0;
    uns8b pixel_buffer[is_color_mapped ? map->bytes_per_entry : pixel_size];
    // The actual pixel size of the image, In order not to be confused with the
    // name of the parameter pixel_size, named data element.
    uns8b data_element_size = pixel_format_to_pixel_size(info->pixel_format);

    for (; pixel_count > 0; --pixel_count) {
        if (packet_count == 0) {
            uns8b repetition_count_field;
            if (trp_tga_fread(&repetition_count_field, 1, info) != 1) {
                error_code = TGA_ERROR_FILE_CANNOT_READ;
                break;
            }
            is_run_length_packet = repetition_count_field & 0x80;
            packet_count = (repetition_count_field & 0x7F) + 1;
            if (is_run_length_packet) {
                if (trp_tga_fread(pixel_buffer, pixel_size, info) != pixel_size) {
                    error_code = TGA_ERROR_FILE_CANNOT_READ;
                    break;
                }
                if (is_color_mapped) {
                    // In color mapped image, the pixel as the index value of
                    // the color map. The actual pixel value is found from the
                    // color map.
                    uns16b index = pixel_to_map_index(pixel_buffer);
                    if (try_get_color_from_map(pixel_buffer, index, map)) {
                        error_code = TGA_ERROR_COLOR_MAP_INDEX_FAILED;
                        break;
                    }
                }
            }
        }

        if (is_run_length_packet) {
            memcpy(data, pixel_buffer, data_element_size);
        } else {
            if (trp_tga_fread(data, pixel_size, info) != pixel_size) {
                error_code = TGA_ERROR_FILE_CANNOT_READ;
                break;
            }
            if (is_color_mapped) {
                // Again, in color mapped image, the pixel as the index value of
                // the color map. The actual pixel value is found from the color
                // map.
                uns16b index = pixel_to_map_index(data);
                if (try_get_color_from_map(data, index, map)) {
                    error_code = TGA_ERROR_COLOR_MAP_INDEX_FAILED;
                    break;
                }
            }
        }

        --packet_count;
        data += data_element_size;
    }
    return error_code;
}

static enum tga_error load_image( uns8b **data_out, tga_info *info )
{
    struct tga_header header;
    enum tga_pixel_format pixel_format;
    enum tga_error error_code;

    error_code = load_header(&header, &pixel_format, info);
    if (error_code != TGA_NO_ERROR) {
        return error_code;
    }
    // No need to handle the content of the ID field, so skip directly.
    if (trp_tga_fseek(info, header.id_length)) {
        return TGA_ERROR_FILE_CANNOT_READ;
    }

    bool is_color_mapped = IS_COLOR_MAPPED(header);
    bool is_rle = IS_RLE(header);

    // Handle color map field.
    struct color_map color_map;
    color_map.pixels = NULL;
    size_t map_size = header.map_length * BITS_TO_BYTES(header.map_entry_size);
    if (is_color_mapped) {
        color_map.first_index = header.map_first_entry;
        color_map.entry_count = header.map_length;
        color_map.bytes_per_entry = BITS_TO_BYTES(header.map_entry_size);
        color_map.pixels = (uns8b *)malloc(map_size);
        if (color_map.pixels == NULL) {
            return TGA_ERROR_OUT_OF_MEMORY;
        }
        if (trp_tga_fread(color_map.pixels, map_size, info) != map_size) {
            free(color_map.pixels);
            return TGA_ERROR_FILE_CANNOT_READ;
        }
    } else if (header.map_type == 1) {
        // The image is not color mapped at this time, but contains a color map.
        // So skips the color map data block directly.
        if (trp_tga_fseek(info, map_size)) {
            return TGA_ERROR_FILE_CANNOT_READ;
        }
    }

    uns8b *data;
    error_code = tga_create(&data, info, header.image_width,
                            header.image_height, pixel_format);
    if (error_code != TGA_NO_ERROR) {
        free(color_map.pixels);
        return error_code;
    }

    // Load image data.
    uns8b pixel_size = BITS_TO_BYTES(header.pixel_depth);
    if (is_rle) {
        error_code = decode_data_rle(data, info, pixel_size, is_color_mapped,
                                     &color_map);
    } else {
        error_code = decode_data(data, info, pixel_size, is_color_mapped,
                                 &color_map);
    }
    free(color_map.pixels);
    if (error_code != TGA_NO_ERROR) {
        free(data);
        return error_code;
    }

    // Flip the image if necessary, to keep the origin in upper left corner.
    bool flip_h = header.image_descriptor & 0x10;
    bool flip_v = !(header.image_descriptor & 0x20);
    if (flip_h)
        tga_image_flip_h(data, info);
    if (flip_v)
        tga_image_flip_v(data, info);

    *data_out = data;
    return TGA_NO_ERROR;
}

// Returns the pixel at coordinates (x,y) for reading or writing.
// If the pixel coordinates are out of bounds (larger than width/height
// or small than 0), they will be clamped.
static inline uns8b *get_pixel(uns8b *data, tga_info *info, int x, int y)
{
    if (x < 0) {
        x = 0;
    } else if (x >= info->width) {
        x = info->width - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= info->height) {
        y = info->height - 1;
    }
    int pixel_size = pixel_format_to_pixel_size(info->pixel_format);
    return data + (y * info->width + x) * pixel_size;
}

static enum tga_error save_image( uns8b *data, tga_info *info )
{
    int pixel_size = pixel_format_to_pixel_size( info->pixel_format );
    uns8b header[HEADER_SIZE];

    memset(header, 0, HEADER_SIZE);
    if (info->pixel_format == TGA_PIXEL_BW8 ||
        info->pixel_format == TGA_PIXEL_BW16) {
        header[2] = (uns8b)TGA_TYPE_GRAYSCALE;
    } else
        header[2] = (uns8b)TGA_TYPE_TRUE_COLOR;
    header[12] = info->width & 0xFF;
    header[13] = (info->width >> 8) & 0xFF;
    header[14] = info->height & 0xFF;
    header[15] = (info->height >> 8) & 0xFF;
    header[16] = pixel_size * 8;
    if ( info->pixel_format == TGA_PIXEL_ARGB32 )
        header[17] = 0x28;
    else
        header[17] = 0x20;

    if ( fwrite( header, 1, HEADER_SIZE, info->fp ) != HEADER_SIZE )
        return TGA_ERROR_FILE_CANNOT_WRITE;

    size_t data_size = (size_t)info->width * info->height * pixel_size;

    if ( fwrite( data, 1, data_size, info->fp ) != data_size )
        return TGA_ERROR_FILE_CANNOT_WRITE;
    return TGA_NO_ERROR;
}

static uns8b trp_pix_load_tga_low( tga_info *info, uns32b *w, uns32b *h, uns8b **data )
{
    uns8b *d;
    uns32b size;
    uns8b res = 1;

    if ( load_image( &d, info ) != TGA_NO_ERROR )
        return 1;
    *w = info->width;
    *h = info->height;
    size = ( ( *w * *h ) << 2 );
    switch ( info->pixel_format ) {
        case TGA_PIXEL_BW8:
            if ( *data = malloc( size ) ) {
                trp_pix_color_t *t = (trp_pix_color_t *)( *data );
                uns8b *c = d;
                uns32b i;

                for ( i = *w * *h ; i ; i--, t++ ) {
                    t->red = t->green = t->blue = *c++;
                    t->alpha = 0xff;
                }
                res = 0;
            }
            break;
        case TGA_PIXEL_BW16:
            /*
             * FIXME
             * va testato
             */
            if ( *data = malloc( size ) ) {
                trp_pix_color_t *t = (trp_pix_color_t *)( *data );
                uns32b *c = (uns32b *)d;
                uns32b i;

                for ( i = *w * *h ; i ; i--, t++ ) {
                    t->red = t->green = t->blue = ( *c++ ) >> 8;
                    t->alpha = 0xff;
                }
                res = 0;
            }
            break;
        case TGA_PIXEL_RGB555:
            /*
             * FIXME
             */
            break;
        case TGA_PIXEL_RGB24:
            if ( *data = malloc( size ) ) {
                trp_pix_color_t *t = (trp_pix_color_t *)( *data );
                uns8b *c = d;
                uns32b i;

                for ( i = *w * *h ; i ; i--, t++ ) {
                    t->blue = *c++;
                    t->green = *c++;
                    t->red = *c++;
                    t->alpha = 0xff;
                }
                res = 0;
            }
            break;
        case TGA_PIXEL_ARGB32:
            if ( *data = malloc( size ) ) {
                trp_pix_color_t *t = (trp_pix_color_t *)( *data );
                uns8b *c = d;
                uns32b i;

                for ( i = *w * *h ; i ; i--, t++ ) {
                    t->blue = *c++;
                    t->green = *c++;
                    t->red = *c++;
                    /*
                     * FIXME
                     * vi sono casi in cui alpha è invertito
                     */
                    t->alpha = *c++;
                }
                res = 0;
            }
            break;
    }
    free( d );
    return res;
}

uns8b trp_pix_load_tga( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    FILE *fp;
    tga_info info;
    uns8b res;

    if ( ( fp = trp_fopen( cpath, "rb" ) ) == NULL )
        return 1;
    info.fp = fp;
    res = trp_pix_load_tga_low( &info, w, h, data );
    fclose( fp );
    return res;
}

uns8b trp_pix_load_tga_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    tga_info info;

    info.fp = NULL;
    info.idata = idata;
    info.isize = isize;
    info.iread = 0;
    return trp_pix_load_tga_low( &info, w, h, data );
}

uns8b trp_pix_save_tga( trp_obj_t *pix, trp_obj_t *path )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns8b *data, *cpath, *d;
    uns32b w, h, i;
    enum tga_pixel_format format;
    tga_info info;
    uns8b res = 1;

    if ( c == NULL )
        return 1;
    cpath = trp_csprint( path );
    info.fp = trp_fopen( cpath, "wb" );
    trp_csprint_free( cpath );
    if ( info.fp == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( trp_pix_has_alpha_low( (trp_pix_t *)pix ) )
        format = TGA_PIXEL_ARGB32;
    else if ( ( trp_pix_colors_type( (trp_pix_t *)pix, 0 ) & 0xfffe ) != 0xfffe )
        format = TGA_PIXEL_RGB24;
    else
        format = TGA_PIXEL_BW8;
    if ( tga_create( &data, &info, w, h, format ) != TGA_NO_ERROR ) {
        fclose( info.fp );
        return 1;
    }
    switch ( info.pixel_format ) {
        case TGA_PIXEL_BW8:
            for ( i = w * h, d = data ; i ; i--, c++ )
                *d++ = c->red;
            break;
        case TGA_PIXEL_RGB24:
            for ( i = w * h, d = data ; i ; i--, c++ ) {
                *d++ = c->blue;
                *d++ = c->green;
                *d++ = c->red;
            }
            break;
        case TGA_PIXEL_ARGB32:
            for ( i = w * h, d = data ; i ; i--, c++ ) {
                *d++ = c->blue;
                *d++ = c->green;
                *d++ = c->red;
                *d++ = c->alpha;
            }
            break;
    }
    if ( save_image( data, &info ) == TGA_NO_ERROR )
        res = 0;
    free( data );
    fclose( info.fp );
    if ( res )
        trp_remove( path );
    return res;
}


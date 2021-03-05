/*
    TreeP Run Time Support
    Copyright (C) 2008-2021 Frank Sinapsi

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
#include "./trpexif.h"
#include <libexif/exif-data.h>

uns8b trp_exif_init()
{
    return 0;
}

trp_obj_t *trp_exif_file( trp_obj_t *path )
{
    uns8b *p = trp_csprint( path );
    ExifData *ed;

    ed = exif_data_new_from_file( trp_get_short_path_name( p ) );
    trp_csprint_free( p );
    if ( ed == NULL )
        return UNDEF;

//    exif_data_dump( ed );

    exif_data_free( ed );
    /*
     FIXME
     */
    return UNDEF;
}


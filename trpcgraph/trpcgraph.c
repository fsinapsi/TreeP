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

#include "../trp/trp.h"
#include "./trpcgraph.h"
#include <gvc.h>
#include <cgraph.h>

uns8b trp_ag_init()
{
    return 0;
}

trp_obj_t *trp_ag_dot2svg( trp_obj_t *s )
{
    Agraph_t *g;
    GVC_t *gvc;
    uns8b *res;
    unsigned len;

    res = trp_csprint( s );
    g = agmemread( res );
    trp_csprint_free( res );
    if ( g == NULL )
        return UNDEF;
    gvc = gvContext();
    gvLayout( gvc, g, "dot" );
    gvRenderData( gvc, g, "svg", (char **)(&res), &len );
    gvFreeLayout( gvc, g );
    agclose( g );
    gvFreeContext( gvc );
    if ( len ) {
        extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );

        s = trp_raw_internal( len, 0 );
        memcpy( ((trp_raw_t *)s)->data, res, len );
    } else
        s = UNDEF;
    gvFreeRenderData( (char *)res );
    return s;
}


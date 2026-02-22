/*
    TreeP Run Time Support
    Copyright (C) 2008-2026 Frank Sinapsi

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

#include "trp.h"

uns8b trp_netptr_print( trp_print_t *p, trp_funptr_t *obj )
{
    if ( trp_print_char_star( p, "#net " ) )
        return 1;
    if ( trp_print_obj( p, obj->name ) )
        return 1;
    return trp_print_char( p, '#' );
}

trp_obj_t *trp_netptr_equal( trp_funptr_t *o1, trp_funptr_t *o2 )
{
    return ( o1->f == o2->f ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_netptr_less( trp_funptr_t *o1, trp_funptr_t *o2 )
{
    return trp_less( o1->name, o2->name );
}

trp_obj_t *trp_netptr_length( trp_funptr_t *fptr )
{
    return trp_sig64( fptr->nargs );
}

trp_obj_t *trp_netptr( uns8bfun_t f, uns8b nargs, trp_obj_t *name )
{
    trp_netptr_t *obj;

    obj = trp_gc_malloc( sizeof( trp_netptr_t ) );
    obj->tipo = TRP_NETPTR;
    obj->f = f;
    obj->nargs = nargs;
    obj->name = name;
    return (trp_obj_t *)obj;
}

uns8b trp_netptr_call( trp_obj_t *nptr, ... )
{
    va_list args;
    uns8b res, nargs = 0;
    trp_obj_t *par, *p[ 25 ];

    if ( nptr->tipo != TRP_NETPTR )
        return 1;
    va_start( args, nptr );
    for (  par = va_arg( args, trp_obj_t * ) ;
           par ;
           par = va_arg( args, trp_obj_t * ) ) {
        if ( nargs == 25 )
            return 1;
        p[ nargs++ ] = par;
    }
    va_end( args );
    if ( ((trp_netptr_t *)nptr)->nargs != nargs )
        return 1;
    switch ( nargs ) {
    case 0:
        res = (((trp_netptr_t *)nptr)->f)();
        break;
    case 1:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ] );
        break;
    case 2:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ] );
        break;
    case 3:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ] );
        break;
    case 4:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ]);
        break;
    case 5:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ] );
        break;
    case 6:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ] );
        break;
    case 7:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ] );
        break;
    case 8:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ] );
        break;
    case 9:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ] );
        break;
    case 10:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ] );
        break;
    case 11:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ] );
        break;
    case 12:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ] );
        break;
    case 13:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ] );
        break;
    case 14:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ] );
        break;
    case 15:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ] );
        break;
    case 16:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ] );
        break;
    case 17:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ] );
        break;
    case 18:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ] );
        break;
    case 19:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ] );
        break;
    case 20:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ] );
        break;
    case 21:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ],
                                           p[ 20 ] );
        break;
    case 22:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ],
                                           p[ 20 ], p[ 21 ] );
        break;
    case 23:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ],
                                           p[ 20 ], p[ 21 ], p[ 22 ] );
        break;
    case 24:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ],
                                           p[ 20 ], p[ 21 ], p[ 22 ], p[ 23 ] );
        break;
    case 25:
        res = (((trp_netptr_t *)nptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ],
                                           p[ 20 ], p[ 21 ], p[ 22 ], p[ 23 ], p[ 24 ] );
        break;
    }
    return res;
}


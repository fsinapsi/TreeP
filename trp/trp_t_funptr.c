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

uns8b trp_funptr_print( trp_print_t *p, trp_funptr_t *obj )
{
    if ( trp_print_char_star( p, "#fun " ) )
        return 1;
    if ( trp_print_obj( p, obj->name ) )
        return 1;
    if ( trp_print_char_star( p, " (" ) )
        return 1;
    if ( trp_print_sig64( p, obj->nargs ) )
        return 1;
    return trp_print_char_star( p, ")#" );
}

trp_obj_t *trp_funptr_equal( trp_funptr_t *o1, trp_funptr_t *o2 )
{
    return ( o1->f == o2->f ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_funptr_less( trp_funptr_t *o1, trp_funptr_t *o2 )
{
    return trp_less( o1->name, o2->name );
}

trp_obj_t *trp_funptr_length( trp_funptr_t *fptr )
{
    return trp_sig64( fptr->nargs );
}

trp_obj_t *trp_funptr( objfun_t f, uns8b nargs, trp_obj_t *name )
{
    trp_funptr_t *obj;

    obj = trp_gc_malloc( sizeof( trp_funptr_t ) );
    obj->tipo = TRP_FUNPTR;
    obj->f = f;
    obj->nargs = nargs;
    obj->name = name;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_funptr_equal_obj()
{
    static trp_obj_t *obj = NULL;

    if ( obj == NULL )
        obj = trp_funptr( trp_equal, 2, trp_cord( "equal" ) );
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_funptr_less_obj()
{
    static trp_obj_t *obj = NULL;

    if ( obj == NULL )
        obj = trp_funptr( trp_less, 2, trp_cord( "less" ) );
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_funptr_call( trp_obj_t *fptr, ... )
{
    va_list args;
    uns8b nargs = 0;
    trp_obj_t *res, *p[ 20 ];

    if ( fptr->tipo != TRP_FUNPTR )
        return UNDEF;

    va_start( args, fptr );
    for (  res = va_arg( args, trp_obj_t * ) ;
           res ;
           res = va_arg( args, trp_obj_t * ) ) {
        if ( nargs == 20 )
            return UNDEF;
        p[ nargs++ ] = res;
    }
    va_end( args );
    if ( ((trp_funptr_t *)fptr)->nargs != nargs )
        return UNDEF;
    switch ( nargs ) {
    case 0:
        res = (((trp_funptr_t *)fptr)->f)();
        break;
    case 1:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ] );
        break;
    case 2:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ] );
        break;
    case 3:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ] );
        break;
    case 4:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ]);
        break;
    case 5:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ] );
        break;
    case 6:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ] );
        break;
    case 7:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ] );
        break;
    case 8:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ] );
        break;
    case 9:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ] );
        break;
    case 10:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ] );
        break;
    case 11:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ] );
        break;
    case 12:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ] );
        break;
    case 13:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ] );
        break;
    case 14:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ] );
        break;
    case 15:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ] );
        break;
    case 16:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ] );
        break;
    case 17:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ] );
        break;
    case 18:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ] );
        break;
    case 19:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ] );
        break;
    case 20:
        res = (((trp_funptr_t *)fptr)->f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                                           p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                                           p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                                           p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ] );
        break;
    }
    return res;
}


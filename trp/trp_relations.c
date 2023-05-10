/*
    TreeP Run Time Support
    Copyright (C) 2008-2023 Frank Sinapsi

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

trp_obj_t *trp_equal( trp_obj_t *o1, trp_obj_t *o2 )
{
    extern objfun_t _trp_equal_fun[];

    if ( o1 == o2 )
        return TRP_TRUE;
    if ( o1->tipo != o2->tipo )
        return TRP_FALSE;
    return (_trp_equal_fun[ o1->tipo ])( o1, o2 );
}

trp_obj_t *trp_notequal( trp_obj_t *o1, trp_obj_t *o2 )
{
    return ( trp_equal( o1, o2 ) == TRP_TRUE ) ? TRP_FALSE : TRP_TRUE;
}

trp_obj_t *trp_less( trp_obj_t *o1, trp_obj_t *o2 )
{
    extern objfun_t _trp_less_fun[];

    if ( o1 == o2 )
        return TRP_FALSE;
    if ( o1->tipo != o2->tipo ) {
        if ( ( ( o1->tipo == TRP_SIG64 ) || ( o1->tipo == TRP_MPI ) ||
               ( o1->tipo == TRP_RATIO ) || ( o1->tipo == TRP_COMPLEX ) ) &&
             ( ( o2->tipo == TRP_SIG64 ) || ( o2->tipo == TRP_MPI ) ||
               ( o2->tipo == TRP_RATIO ) || ( o2->tipo == TRP_COMPLEX ) ) )
            return trp_math_less( o1, o2 );
        return ( o1->tipo < o2->tipo ) ? TRP_TRUE : TRP_FALSE;
    }
    return (_trp_less_fun[ o1->tipo ])( o1, o2 );
}

trp_obj_t *trp_greater( trp_obj_t *o1, trp_obj_t *o2 )
{
    return trp_less( o2, o1 );
}

trp_obj_t *trp_less_or_equal( trp_obj_t *o1, trp_obj_t *o2 )
{
    if ( trp_less( o1, o2 ) == TRP_TRUE )
        return TRP_TRUE;
    return trp_equal( o1, o2 );
}

trp_obj_t *trp_greater_or_equal( trp_obj_t *o1, trp_obj_t *o2 )
{
    if ( trp_less( o2, o1 ) == TRP_TRUE )
        return TRP_TRUE;
    return trp_equal( o1, o2 );
}

trp_obj_t *trp_min( trp_obj_t *obj, ... )
{
    trp_obj_t *res = obj;
    va_list args;

    va_start( args, obj );
    for ( obj = va_arg( args, trp_obj_t * ) ;
          obj ;
          obj = va_arg( args, trp_obj_t * ) )
        if ( trp_less( obj, res ) == TRP_TRUE )
            res = obj;
    va_end( args );
    return res;
}

trp_obj_t *trp_max( trp_obj_t *obj, ... )
{
    trp_obj_t *res = obj;
    va_list args;

    va_start( args, obj );
    for ( obj = va_arg( args, trp_obj_t * ) ;
          obj ;
          obj = va_arg( args, trp_obj_t * ) )
        if ( trp_less( res, obj ) == TRP_TRUE )
            res = obj;
    va_end( args );
    return res;
}

trp_obj_t *trp_or( trp_obj_t *obj, ... )
{
    va_list args;

    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        if ( obj == TRP_TRUE ) {
            va_end( args );
            return TRP_TRUE;
        }
    va_end( args );
    return TRP_FALSE;
}

trp_obj_t *trp_and( trp_obj_t *obj, ... )
{
    va_list args;

    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        if ( obj == TRP_FALSE ) {
            va_end( args );
            return TRP_FALSE;
        }
    va_end( args );
    return TRP_TRUE;
}

trp_obj_t *trp_not( trp_obj_t *obj )
{
    return ( obj == TRP_FALSE ) ? TRP_TRUE : TRP_FALSE;
}


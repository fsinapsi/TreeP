/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

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

uns8b trp_cast_uns32b( trp_obj_t *obj, uns32b *val )
{
    if ( obj->tipo != TRP_SIG64 )
        return 1;
    if ( ( ((trp_sig64_t *)obj)->val < 0 ) ||
         ( ((trp_sig64_t *)obj)->val > 0xffffffff ) )
        return 1;
    *val = (uns32b)( ((trp_sig64_t *)obj)->val );
    return 0;
}

uns8b trp_cast_uns32b_range( trp_obj_t *obj, uns32b *val, uns32b min, uns32b max )
{
    if ( trp_cast_uns32b( obj, val ) )
        return 1;
    return ( ( *val >= min ) && ( *val <= max ) ) ? 0 : 1;
}

uns8b trp_cast_uns32b_rint( trp_obj_t *obj, uns32b *val )
{
    obj = trp_math_rint( obj );
    if ( obj->tipo != TRP_SIG64 )
        return 1;
    if ( ( ((trp_sig64_t *)obj)->val < 0 ) ||
         ( ((trp_sig64_t *)obj)->val > 0xffffffff ) )
        return 1;
    *val = (uns32b)( ((trp_sig64_t *)obj)->val );
    return 0;
}

uns8b trp_cast_uns32b_rint_range( trp_obj_t *obj, uns32b *val, uns32b min, uns32b max )
{
    if ( trp_cast_uns32b_rint( obj, val ) )
        return 1;
    return ( ( *val >= min ) && ( *val <= max ) ) ? 0 : 1;
}

uns8b trp_cast_sig32b( trp_obj_t *obj, sig32b *val )
{
    if ( obj->tipo != TRP_SIG64 )
        return 1;
    *val = (sig32b)( ((trp_sig64_t *)obj)->val );
    return ( (sig64b)( *val ) == ((trp_sig64_t *)obj)->val ) ? 0 : 1;
}

uns8b trp_cast_sig32b_range( trp_obj_t *obj, sig32b *val, sig32b min, sig32b max )
{
    if ( obj->tipo != TRP_SIG64 )
        return 1;
    *val = (sig32b)( ((trp_sig64_t *)obj)->val );
    return ( ( (sig64b)( *val ) == ((trp_sig64_t *)obj)->val ) &&
             ( *val >= min ) && ( *val <= max ) ) ? 0 : 1;
}

uns8b trp_cast_sig64b( trp_obj_t *obj, sig64b *val )
{
    if ( obj->tipo != TRP_SIG64 )
        return 1;
    *val = ((trp_sig64_t *)obj)->val;
    return 0;
}

uns8b trp_cast_sig64b_range( trp_obj_t *obj, sig64b *val, sig64b min, sig64b max )
{
    if ( trp_cast_sig64b( obj, val ) )
        return 1;
    return ( ( *val >= min ) && ( *val <= max ) ) ? 0 : 1;
}

uns8b trp_cast_sig64b_rint( trp_obj_t *obj, sig64b *val )
{
    obj = trp_math_rint( obj );
    if ( obj->tipo != TRP_SIG64 )
        return 1;
    *val = ((trp_sig64_t *)obj)->val;
    return 0;
}

uns8b trp_cast_sig64b_rint_range( trp_obj_t *obj, sig64b *val, sig64b min, sig64b max )
{
    if ( trp_cast_sig64b_rint( obj, val ) )
        return 1;
    return ( ( *val >= min ) && ( *val <= max ) ) ? 0 : 1;
}

uns8b trp_cast_double( trp_obj_t *obj, double *val )
{
    uns8b res;

    switch ( obj->tipo ) {
    case TRP_SIG64:
        *val = (double)( ((trp_sig64_t *)obj)->val );
        res = 0;
        break;
    case TRP_RATIO:
        *val = mpq_get_d( ((trp_ratio_t *)obj)->val );
        res = 0;
        break;
    default:
        res = 1;
        break;
    }
    return res;
}

uns8b trp_cast_double_range( trp_obj_t *obj, double *val, double min, double max )
{
    if ( trp_cast_double( obj, val ) )
        return 1;
    return ( ( *val >= min ) && ( *val <= max ) ) ? 0 : 1;
}


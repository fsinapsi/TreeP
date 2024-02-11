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
#include "./entities.h"
#include "./c_escape.h"

trp_obj_t *trp_str_clean( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c, *p, *q;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    for ( p = q = c ; *p ; ) {
        if ( *p == '\t' )
            *p = ' ';
        else if ( *p == '\r' )
            *p = '\n';
        else if ( ( *p == 194 ) && ( *(p+1) == 160 ) ) {
            /*
             * si converte un nbsp in uno spazio
             */
            p++;
            *p = ' ';
        }
        if ( ( *p == ' ' ) || ( *p == '\n' ) ) {
            /*
             * si tolgono gli spazi iniziali
             */
            if ( q == c ) {
                p++;
                continue;
            }
            if ( *(q-1) == ' ' ) {
                *(q-1) = *p++;
                continue;
            }
            if ( *(q-1) == '\n' ) {
                p++;
                continue;
            }
        }
        if ( q != c ) {
            /*
             * si tolgono gli spazi che precedono uno di questi caratteri
             */
            if ( ( *p == '.' ) || ( *p == ',' ) || ( *p == ';' ) ||
                 ( *p == ':' ) || ( *p == '!' ) || ( *p == '?' ) ||
                 ( *p == ')' ) || ( *p == ']' ) || ( *p == '}' ) ) {
                if ( *(q-1) == ' ' ) {
                    q--;
                    continue;
                }
            }
            /*
             * si tolgono gli spazi che precedono "»"
             */
            else if ( *p == 187 ) {
                if ( *(q-1) == 194 )
                    if ( (q-1) != c )
                        if ( *(q-2) == ' ' ) {
                            *(q-2) = 194;
                            *(q-1) = *p++;
                            continue;
                        }
            }
            /*
             * si tolgono gli spazi che precedono "”"
             */
            else if ( *p == 157 ) {
                if ( *(q-1) == 128 )
                    if ( (q-1) != c )
                        if ( *(q-2) == 226 )
                            if ( (q-2) != c )
                                if ( *(q-3) == ' ' ) {
                                    *(q-3) = 226;
                                    *(q-2) = 128;
                                    *(q-1) = *p++;
                                    continue;
                                }
            }
            /*
             * si tolgono gli spazi successivi a una parentesi aperta o a "«" o a "“"
             */
            else if ( *p == ' ' ) {
                if ( ( *(q-1) == '(' ) || ( *(q-1) == '[' ) || ( *(q-1) == '{' ) ) {
                    p++;
                    continue;
                }
                if ( *(q-1) == 171 )
                    if ( (q-1) != c )
                        if ( *(q-2) == 194 ) {
                            p++;
                            continue;
                        }
                if ( *(q-1) == 156 )
                    if ( (q-1) != c )
                        if ( *(q-2) == 128 )
                            if ( (q-2) != c )
                                if ( *(q-3) == 226 ) {
                                    p++;
                                    continue;
                                }
            }
            /*
             * gestione dei punti di sospensione
             */
            if ( *p == '.' ) {
                if ( *(q-1) == '.' ) {
                    if ( q-1 != c )
                        if ( *(q-2) == '.' ) {
                            /*
                             * si sostituiscono 3 punti con
                             * un punto di sospensione utf-8
                             */
                            *(q-2) = 226;
                            *(q-1) = 128;
                            *q++ = 166;
                            p++;
                            continue;
                        }
                } else if ( *(q-1) == 166 )
                    if ( q-1 != c )
                        if ( *(q-2) == 128 )
                            if ( q-2 != c )
                                if ( *(q-3) == 226 ) {
                                    /*
                                     * si ignorano i punti successivi
                                     * a un punto di sospensione utf-8
                                     */
                                    p++;
                                    continue;
                                }
            }
        }
        *q++ = *p++;
    }
    /*
     * si tolgono gli spazi finali
     */
    if ( q != c )
        if ( ( *(q-1) == ' ' ) || ( *(q-1) == '\n' ) )
            q--;
    if ( q == c ) {
        trp_csprint_free( c );
        return EMPTYCORD;
    }
    *q = '\0';
    s = trp_cord( c );
    trp_csprint_free( c );
    return s;
}

trp_obj_t *trp_str_uniform_blanks( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c, *p;
    CORD_ec x;
    uns32b len = 0;
    uns8b prv_is_space = 0;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    CORD_ec_init( x );
    for ( p = c ; *p ; ) {
        if ( *p == '\t' )
            *p = ' ';
        else if ( *p == '\r' )
            *p = ' ';
        else if ( *p == '\n' )
            *p = ' ';
        else if ( ( *p == 194 ) && ( *(p+1) == 160 ) ) {
            /*
             * si converte un nbsp in uno spazio
             */
            p++;
            *p = ' ';
        }
        if ( *p == ' ' ) {
            if ( prv_is_space ) {
                p++;
                continue;
            }
            prv_is_space = 1;
        } else {
            prv_is_space = 0;
        }
        CORD_ec_append( x, *p++ );
        len++;
    }
    trp_csprint_free( c );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_str_brackets_are_balanced( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c, *p;
    uns32b tc, tq, tg, tn, tv;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    s = TRP_FALSE;
    for ( p = c, tc = 0, tq = 0, tg = 0, tn = 0, tv = 0 ; *p ; p++ ) {
        switch ( *p ) {
            case '(':
                tc++;
                break;
            case ')':
                if ( tc == 0 )
                    goto brackets_exit;
                tc--;
                break;
            case '[':
                tq++;
                break;
            case ']':
                if ( tq == 0 )
                    goto brackets_exit;
                tq--;
                break;
            case '{':
                tg++;
                break;
            case '}':
                if ( tg == 0 )
                    goto brackets_exit;
                tg--;
                break;
            case 194: /* "«" "»" */
                if ( *(p+1) == 171 )
                    tn++;
                else if ( *(p+1) == 187 ) {
                    if ( tn == 0 )
                        goto brackets_exit;
                    tn--;
                }
                break;
            case 226: /* "“" "”" */
                if ( *(p+1) == 128 )
                    if ( *(p+2) == 156 )
                        tv++;
                    else if ( *(p+2) == 157 ) {
                        if ( tv == 0 )
                            goto brackets_exit;
                        tv--;
                    }
                break;
        }
    }
    if ( ( tc == 0 ) && ( tq == 0 ) && ( tg == 0 ) && ( tn == 0 ) && ( tv == 0 ) )
        s = TRP_TRUE;
brackets_exit:
    trp_csprint_free( c );
    return s;
}

trp_obj_t *trp_str_decode_html_entities_utf8( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    if ( *c == '\0' ) {
        trp_csprint_free( c );
        return EMPTYCORD;
    }
    decode_html_entities_utf8( c, NULL );
    s = trp_cord( c );
    trp_csprint_free( c );
    return s;
}

trp_obj_t *trp_str_encode_html_entities( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c, *d;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    CORD_ec x;
    uns32b len = 0;

    CORD_ec_init( x );
    for ( d = c ; *d ; d++ ) {
        if ( *d == '&' ) {
            CORD_ec_append( x, '&' );
            CORD_ec_append( x, 'a' );
            CORD_ec_append( x, 'm' );
            CORD_ec_append( x, 'p' );
            CORD_ec_append( x, ';' );
            len += 5;
            continue;
        }
        if ( *d == '<' ) {
            CORD_ec_append( x, '&' );
            CORD_ec_append( x, 'l' );
            CORD_ec_append( x, 't' );
            CORD_ec_append( x, ';' );
            len += 4;
            continue;
        }
        if ( *d == '>' ) {
            CORD_ec_append( x, '&' );
            CORD_ec_append( x, 'g' );
            CORD_ec_append( x, 't' );
            CORD_ec_append( x, ';' );
            len += 4;
            continue;
        }
        /*
        if ( *d == '\'' ) {
            CORD_ec_append( x, '&' );
            CORD_ec_append( x, 'a' );
            CORD_ec_append( x, 'p' );
            CORD_ec_append( x, 'o' );
            CORD_ec_append( x, 's' );
            CORD_ec_append( x, ';' );
            len += 6;
            continue;
        }
        if ( *d == '"' ) {
            CORD_ec_append( x, '&' );
            CORD_ec_append( x, 'q' );
            CORD_ec_append( x, 'u' );
            CORD_ec_append( x, 'o' );
            CORD_ec_append( x, 't' );
            CORD_ec_append( x, ';' );
            len += 6;
            continue;
        }
        */
        CORD_ec_append( x, *d );
        len++;
    }
    trp_csprint_free( c );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_str_decode_url( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c, *d;
    CORD_ec x;
    uns32b len = 0;
    uns8b e, f;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    CORD_ec_init( x );
    for ( d = c ; ; ) {
        e = *d++;
        if ( e == 0 )
            break;
        if ( e == '%' ) {
            f = *d++;
            if ( ( f >= '0' ) && ( f <= '9' ) )
                e = f - '0';
            else if ( ( f >= 'A' ) && ( f <= 'F' ) )
                e = ( f - 'A' ) + 10;
            else if ( ( f >= 'a' ) && ( f <= 'f' ) )
                e = ( f - 'a' ) + 10;
            else
                goto decode_url_error;
            e <<= 4;
            f = *d++;
            if ( ( f >= '0' ) && ( f <= '9' ) )
                e |= f - '0';
            else if ( ( f >= 'A' ) && ( f <= 'F' ) )
                e |= ( f - 'A' ) + 10;
            else if ( ( f >= 'a' ) && ( f <= 'f' ) )
                e |= ( f - 'a' ) + 10;
            else
                goto decode_url_error;
        }
        CORD_ec_append( x, e );
        len++;
    }
    trp_csprint_free( c );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
decode_url_error:
    trp_csprint_free( c );
    return UNDEF;
}

trp_obj_t *trp_str_encode_url( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c, *d;
    CORD_ec x;
    uns32b len = 0;
    uns8b e, f;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    CORD_ec_init( x );
    for ( d = c ; ; ) {
        e = *d++;
        if ( e == 0 )
            break;
        if ( ( ( e >= '0' ) && ( e <= '9' ) ) ||
             ( ( e >= 'A' ) && ( e <= 'Z' ) ) ||
             ( ( e >= 'a' ) && ( e <= 'z' ) ) ) {
            CORD_ec_append( x, e );
            len++;
        } else {
            CORD_ec_append( x, '%' );
            f = e >> 4;
            CORD_ec_append( x, ( f < 10 ) ? ( '0' + f ) : ( 'A' + f - 10 ) );
            f = e & 15;
            CORD_ec_append( x, ( f < 10 ) ? ( '0' + f ) : ( 'A' + f - 10 ) );
            len += 3;
        }
    }
    trp_csprint_free( c );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_str_json_unescape( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c, *d;
    int error_pos;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    d = unescape( c, CE_JSON, &error_pos );
    if ( d ) {
        s = trp_cord( d );
        free( d );
    } else
        s = UNDEF;
    trp_csprint_free( c );
    return s;
}

trp_obj_t *trp_str_json_escape( trp_obj_t *s, ... )
{
    va_list args;
    uns8b *c, *d;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    d = escape( c, CE_JSON );
    if ( d ) {
        s = trp_cord( d );
        free( d );
    } else
        s = UNDEF;
    trp_csprint_free( c );
    return s;
}


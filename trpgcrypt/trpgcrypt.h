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

#ifndef __trpgcrypt__h
#define __trpgcrypt__h

#include <gcrypt.h>

uns8b trp_gcry_init();
void trp_gcry_quit();
trp_obj_t *trp_gcry_version();
trp_obj_t *trp_gcry_permute( trp_obj_t *size, trp_obj_t *pass_phrase, trp_obj_t *index );
trp_obj_t *trp_gcry_permute_inv( trp_obj_t *size, trp_obj_t *pass_phrase, trp_obj_t *index );
uns8b trp_gcry_stego_insert( trp_obj_t *obj, trp_obj_t *pass_phrase, trp_obj_t *msg );
trp_obj_t *trp_gcry_stego_extract( trp_obj_t *obj, trp_obj_t *pass_phrase );
uns8b trp_gcry_stego_destroy( trp_obj_t *obj, trp_obj_t *pass_phrase );
trp_obj_t *trp_gcry_md_hash( trp_obj_t *algo, trp_obj_t *obj );
trp_obj_t *trp_gcry_md_hash_fast( trp_obj_t *algo, trp_obj_t *obj );
trp_obj_t *trp_gcry_md_hash_file( trp_obj_t *algo, trp_obj_t *path );

#endif /* !__trpgcrypt__h */

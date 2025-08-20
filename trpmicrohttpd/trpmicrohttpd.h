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

#ifndef __trpmicrohttpd__h
#define __trpmicrohttpd__h

#include <microhttpd.h>

uns8b trp_mhd_init();
trp_obj_t *trp_mhd_version();
trp_obj_t *trp_mhd_is_feature_supported( trp_obj_t *feat );
trp_obj_t *trp_mhd_start_daemon( trp_obj_t *cbfun, trp_obj_t *port );
uns8b trp_mhd_run_wait( trp_obj_t *obj, trp_obj_t *timeout );
uns8b trp_mhd_stop_daemon( trp_obj_t *obj );
trp_obj_t *trp_mhd_get_connection_info_client_address( trp_obj_t *conn );

#endif /* !__trpmicrohttpd__h */

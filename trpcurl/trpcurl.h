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

#ifndef __trpcurl__h
#define __trpcurl__h

#include <curl/curl.h>

uns8b trp_curl_init();
void trp_curl_quit();
trp_obj_t *trp_curl_version();
trp_obj_t *trp_curl_easy_init();
trp_obj_t *trp_curl_easy_escape( trp_obj_t *curl, trp_obj_t *url );
trp_obj_t *trp_curl_easy_unescape( trp_obj_t *curl, trp_obj_t *url );
uns8b trp_curl_easy_reset( trp_obj_t *curl );
uns8b trp_curl_easy_perform( trp_obj_t *curl );
trp_obj_t *trp_curl_easy_getinfo_errorbuffer( trp_obj_t *curl );
trp_obj_t *trp_curl_easy_getinfo_effective_url( trp_obj_t *curl );
trp_obj_t *trp_curl_easy_getinfo_response_code( trp_obj_t *curl );
trp_obj_t *trp_curl_easy_getinfo_http_connectcode( trp_obj_t *curl );
trp_obj_t *trp_curl_easy_getinfo_filetime( trp_obj_t *curl );
trp_obj_t *trp_curl_easy_getinfo_size_download( trp_obj_t *curl );
trp_obj_t *trp_curl_easy_getinfo_size_upload( trp_obj_t *curl );
uns8b trp_curl_easy_setopt_errorbuffer( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_url( trp_obj_t *curl, trp_obj_t *url );
uns8b trp_curl_easy_setopt_postfields( trp_obj_t *curl, trp_obj_t *s );
uns8b trp_curl_easy_setopt_postfieldsize_large( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_writefunction( trp_obj_t *curl, trp_obj_t *net, trp_obj_t *data );
uns8b trp_curl_easy_setopt_readfunction( trp_obj_t *curl, trp_obj_t *len, trp_obj_t *net, trp_obj_t *data );
uns8b trp_curl_easy_setopt_progressfunction( trp_obj_t *curl, trp_obj_t *net, trp_obj_t *data );
uns8b trp_curl_easy_setopt_filetime( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_proxy( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_pre_proxy( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_proxyport( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_noproxy( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_httpproxytunnel( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_username( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_password( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_proxyusername( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_proxypassword( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_autoreferer( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_referer( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_accept_encoding( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_useragent( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_resume_from( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_httpget( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_post( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_followlocation( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_maxredirs( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_crlf( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_cookie( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_cookiefile( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_cookiejar( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_cookiesession( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_sslcert( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_ssl_cipher_list( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_keypasswd( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_capath( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_ssl_verifypeer( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_cainfo( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_ftpport( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_interface( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_customrequest( trp_obj_t *curl, trp_obj_t *val );
/*
uns8b trp_curl_easy_setopt_krblevel( trp_obj_t *curl, trp_obj_t *val );
*/
uns8b trp_curl_easy_setopt_nobody( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_quote( trp_obj_t *curl, ... );
uns8b trp_curl_easy_setopt_postquote( trp_obj_t *curl, ... );
uns8b trp_curl_easy_setopt_prequote( trp_obj_t *curl, ... );
uns8b trp_curl_easy_setopt_httpheader( trp_obj_t *curl, ... );
uns8b trp_curl_easy_setopt_header( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_stderr( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_verbose( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_failonerror( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_ignore_content_length( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_ssh_public_keyfile( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_ssh_private_keyfile( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_dirlistonly( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_append( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_ftp_account( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_maxconnects( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_timeout_ms( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_fresh_connect( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_forbid_reuse( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_tcp_keepalive( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_tcp_nodelay( trp_obj_t *curl, trp_obj_t *set_on_off );
uns8b trp_curl_easy_setopt_http_version( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_expect_100_timeout_ms( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_use_ssl( trp_obj_t *curl, trp_obj_t *val );
uns8b trp_curl_easy_setopt_low_speed( trp_obj_t *curl, trp_obj_t *speed_limit, trp_obj_t *speed_time );

#endif /* !__trpcurl__h */

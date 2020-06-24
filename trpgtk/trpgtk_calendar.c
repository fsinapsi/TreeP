/*
    TreeP Run Time Support
    Copyright (C) 2008-2020 Frank Sinapsi

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

#include "./trpgtk_internal.h"

trp_obj_t *trp_gtk_calendar_new()
{
    return trp_gtk_widget( gtk_calendar_new() );
}

void trp_gtk_calendar_set_date( trp_obj_t *obj, trp_obj_t *date )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( date->tipo == TRP_DATE ) )
        if ( GTK_IS_CALENDAR( o ) ) {
            guint year, month, day;

            year = ((trp_date_t *)date)->anno;
            month = ((trp_date_t *)date)->mese;
            day = ((trp_date_t *)date)->giorno;
            if ( month ) {
                gtk_calendar_select_month( (GtkCalendar *)o, month - 1, year );
                gtk_calendar_select_day( (GtkCalendar *)o, day );
            }
        }
}

trp_obj_t *trp_gtk_calendar_get_date( trp_obj_t *obj )
{
    extern trp_obj_t *trp_date_internal( uns16b anno, uns8b mese, uns8b giorno, uns8b ore, uns8b minuti, uns8b secondi, trp_obj_t *resto, sig32b tz );
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_CALENDAR( o ) ) {
            guint year, month, day;

            gtk_calendar_get_date( (GtkCalendar *)o, &year, &month, &day );
            res = trp_date_internal( year, month + 1, day, 24, 60, 60, ZERO, 0 );
        }
    return res;
}




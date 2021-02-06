/*
  Miscellaneous utility functions.

  Copyright (C) 2006-2010  Rob Hess <hess@eecs.oregonstate.edu>

  @version 1.1.2-20100521
*/

#include "utils.h"

#include <cv.h>
#include <cxcore.h>
// #include <highgui.h>
#include <opencv2/highgui/highgui_c.h>

/*
#include <gdk/gdk.h>
#include <gtk/gtk.h>
*/

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/*************************** Function Definitions ****************************/

/*
  Prints an error message and aborts the program.  The error message is
  of the form "Error: ...", where the ... is specified by the \a format
  argument

  @param format an error message format string (as with \c printf(3)).
*/
void fatal_error(char* format, ...)
{
  va_list ap;

  fprintf( stderr, "Error: ");

  va_start( ap, format );
  vfprintf( stderr, format, ap );
  va_end( ap );
  fprintf( stderr, "\n" );
  abort();
}

/*
  Doubles the size of an array with error checking

  @param array pointer to an array whose size is to be doubled
  @param n number of elements allocated for \a array
  @param size size in bytes of elements in \a array

  @return Returns the new number of elements allocated for \a array.  If no
    memory is available, returns 0.
*/
int array_double( void** array, int n, int size )
{
  void* tmp;

  tmp = realloc( *array, 2 * n * size );
  if( ! tmp )
    {
      fprintf( stderr, "Warning: unable to allocate memory in array_double(),"
               " %s line %d\n", __FILE__, __LINE__ );
      if( *array )
        free( *array );
      *array = NULL;
      return 0;
    }
  *array = tmp;
  return n*2;
}

/*
  Calculates the squared distance between two points.

  @param p1 a point
  @param p2 another point
*/
double dist_sq_2D( CvPoint2D64f p1, CvPoint2D64f p2 )
{
  double x_diff = p1.x - p2.x;
  double y_diff = p1.y - p2.y;

  return x_diff * x_diff + y_diff * y_diff;
}


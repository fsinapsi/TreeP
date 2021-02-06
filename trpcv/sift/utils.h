/**@file
   Miscellaneous utility functions.

   Copyright (C) 2006-2010  Rob Hess <hess@eecs.oregonstate.edu>

   @version 1.1.2-20100521
*/

#ifndef UTILS_H
#define UTILS_H

#include "cxcore.h"

#include <stdio.h>
#include <dirent.h>

/* absolute value */
#ifndef ABS
#define ABS(x) ( ( (x) < 0 )? -(x) : (x) )
#endif

/***************************** Inline Functions ******************************/

/**
   A function to get a pixel value from an 8-bit unsigned image.

   @param img an image
   @param r row
   @param c column
   @return Returns the value of the pixel at (\a r, \a c) in \a img
*/
static inline int pixval8( IplImage* img, int r, int c )
{
  return (int)( ( (uchar*)(img->imageData + img->widthStep*r) )[c] );
}

/**
   A function to set a pixel value in an 8-bit unsigned image.

   @param img an image
   @param r row
   @param c column
   @param val pixel value
*/
static inline void setpix8( IplImage* img, int r, int c, uchar val)
{
  ( (uchar*)(img->imageData + img->widthStep*r) )[c] = val;
}

/**
   A function to get a pixel value from a 32-bit floating-point image.

   @param img an image
   @param r row
   @param c column
   @return Returns the value of the pixel at (\a r, \a c) in \a img
*/
static inline float pixval32f( IplImage* img, int r, int c )
{
  return ( (float*)(img->imageData + img->widthStep*r) )[c];
}

/**
   A function to set a pixel value in a 32-bit floating-point image.

   @param img an image
   @param r row
   @param c column
   @param val pixel value
*/
static inline void setpix32f( IplImage* img, int r, int c, float val )
{
  ( (float*)(img->imageData + img->widthStep*r) )[c] = val;
}

/**
   A function to get a pixel value from a 64-bit floating-point image.

   @param img an image
   @param r row
   @param c column
   @return Returns the value of the pixel at (\a r, \a c) in \a img
*/
static inline double pixval64f( IplImage* img, int r, int c )
{
  return (double)( ( (double*)(img->imageData + img->widthStep*r) )[c] );
}

/**
   A function to set a pixel value in a 64-bit floating-point image.

   @param img an image
   @param r row
   @param c column
   @param val pixel value
*/
static inline void setpix64f( IplImage* img, int r, int c, double val )
{
  ( (double*)(img->imageData + img->widthStep*r) )[c] = val;
}

/**************************** Function Prototypes ****************************/

/**
   Prints an error message and aborts the program.  The error message is
   of the form "Error: ...", where the ... is specified by the \a format
   argument

   @param format an error message format string (as with \c printf(3)).
*/
extern void fatal_error( char* format, ... );

/**
   Doubles the size of an array with error checking

   @param array pointer to an array whose size is to be doubled
   @param n number of elements allocated for \a array
   @param size size in bytes of elements in \a array

   @return Returns the new number of elements allocated for \a array.  If no
     memory is available, returns 0 and frees \a array.
*/
extern int array_double( void** array, int n, int size );

/**
   Calculates the squared distance between two points.

   @param p1 a point
   @param p2 another point
*/
extern double dist_sq_2D( CvPoint2D64f p1, CvPoint2D64f p2 );

#endif

#include "cnetheader.h"

/*  The cnet network simulator (v2.0.10)
    Copyright (C) 1992-2006, Chris McDonald

    Chris McDonald, chris@csse.uwa.edu.au
    Department of Computer Science & Software Engineering
    The University of Western Australia,
    Crawley, Western Australia, 6009
    PH: +61 8 9380 2533, FAX: +61 8 9380 1089.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* DON'T GET EXCITED YET - THE WINDOWS VERSION OF cnet IS NOT YET READY */


#if	defined(USE_WIN32)

/* Copyright (C) 1995, 1996 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, August 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#if	!defined(USHRT_MAX)
/* Maximum value an `unsigned short int' can hold.  (Minimum is 0.)  */
#  define USHRT_MAX     65535
#endif

#if	!defined(NULL)
#define NULL ((void *)0)
#endif

typedef ULONGLONG u_int64_t;

/* Data structure for communication with thread safe versions.  */
struct drand48_data
  {
    unsigned short int x[3];    /* Current state.  */
    unsigned short int a[3];    /* Factor in congruential formula.  */
    unsigned short int c;       /* Additive const. in congruential formula.  */
    unsigned short int old_x[3]; /* Old state.  */
    int init;                   /* Flag for initializing.  */
  };


/* ------------------------- drand48-iter.c ---------------------------- */

/* Global state for non-reentrant functions.  */
static struct drand48_data __libc_drand48_data;

static int
__drand48_iterate (xsubi, buffer)
     unsigned short int xsubi[3];
     struct drand48_data *buffer;
{
  u_int64_t X, a, result;

  /* Initialize buffer, if not yet done.  */
  if (!buffer->init)
    {
#if (USHRT_MAX == 0xffffU)
      buffer->a[2] = 0x5;
      buffer->a[1] = 0xdeec;
      buffer->a[0] = 0xe66d;
#else
      buffer->a[2] = 0x5deecUL;
      buffer->a[1] = 0xe66d0000UL;
      buffer->a[0] = 0;
#endif
      buffer->c = 0xb;
      buffer->init = 1;
    }

  /* Do the real work.  We choose a data type which contains at least
     48 bits.  Because we compute the modulus it does not care how
     many bits really are computed.  */

  if (sizeof (unsigned short int) == 2)
    {
      X = (u_int64_t)xsubi[2] << 32 | (u_int64_t)xsubi[1] << 16 | xsubi[0];
      a = ((u_int64_t)buffer->a[2] << 32 | (u_int64_t)buffer->a[1] << 16
	   | buffer->a[0]);

      result = X * a + buffer->c;

      xsubi[0] = (unsigned short)(result & 0xffff);
      xsubi[1] = (unsigned short)((result >> 16) & 0xffff);
      xsubi[2] = (unsigned short)((result >> 32) & 0xffff);
    }
  else
    {
      X = (u_int64_t)xsubi[2] << 16 | xsubi[1] >> 16;
      a = (u_int64_t)buffer->a[2] << 16 | buffer->a[1] >> 16;

      result = X * a + buffer->c;

      xsubi[0] = (unsigned short)(result >> 16 & 0xffffffffl);
      xsubi[1] = (unsigned short)(result << 16 & 0xffff0000l);
    }

  return 0;
}

/* -------------------------- erand48.c ---------------------------- */

#define __LITTLE_ENDIAN		1234
#define __BIG_ENDIAN		4321

#define __BYTE_ORDER		__LITTLE_ENDIAN
#define __FLOAT_WORD_ORDER	__BYTE_ORDER

union ieee754_double
  {
    double d;

    /* This is the IEEE 754 double-precision format.  */
    struct
      {
#if	__BYTE_ORDER == __BIG_ENDIAN
	unsigned int negative:1;
	unsigned int exponent:11;
	/* Together these comprise the mantissa.  */
	unsigned int mantissa0:20;
	unsigned int mantissa1:32;
#endif				/* Big endian.  */
#if	__BYTE_ORDER == __LITTLE_ENDIAN
# if	__FLOAT_WORD_ORDER == BIG_ENDIAN
	unsigned int mantissa0:20;
	unsigned int exponent:11;
	unsigned int negative:1;
	unsigned int mantissa1:32;
# else
	/* Together these comprise the mantissa.  */
	unsigned int mantissa1:32;
	unsigned int mantissa0:20;
	unsigned int exponent:11;
	unsigned int negative:1;
# endif
#endif				/* Little endian.  */
      } ieee;

    /* This format makes it easier to see if a NaN is a signalling NaN.  */
    struct
      {
#if	__BYTE_ORDER == __BIG_ENDIAN
	unsigned int negative:1;
	unsigned int exponent:11;
	unsigned int quiet_nan:1;
	/* Together these comprise the mantissa.  */
	unsigned int mantissa0:19;
	unsigned int mantissa1:32;
#else
# if	__FLOAT_WORD_ORDER == BIG_ENDIAN
	unsigned int mantissa0:19;
	unsigned int quiet_nan:1;
	unsigned int exponent:11;
	unsigned int negative:1;
	unsigned int mantissa1:32;
# else
	/* Together these comprise the mantissa.  */
	unsigned int mantissa1:32;
	unsigned int mantissa0:19;
	unsigned int quiet_nan:1;
	unsigned int exponent:11;
	unsigned int negative:1;
# endif
#endif
      } ieee_nan;
  };

#define IEEE754_DOUBLE_BIAS	0x3ff /* Added to exponent.  */

static int
__erand48_r (xsubi, buffer, result)
     unsigned short int xsubi[3];
     struct drand48_data *buffer;
     double *result;
{
  union ieee754_double temp;

  /* Compute next state.  */
  if (__drand48_iterate (xsubi, buffer) < 0)
    return -1;

  /* Construct a positive double with the 48 random bits distributed over
     its fractional part so the resulting FP number is [0.0,1.0).  */

#if USHRT_MAX == 65535
  temp.ieee.negative = 0;
  temp.ieee.exponent = IEEE754_DOUBLE_BIAS;
  temp.ieee.mantissa0 = (xsubi[2] << 4) | (xsubi[1] >> 12);
  temp.ieee.mantissa1 = ((xsubi[1] & 0xfff) << 20) | (xsubi[0] << 4);
#elif USHRT_MAX == 2147483647
  temp.ieee.negative = 0;
  temp.ieee.exponent = IEEE754_DOUBLE_BIAS;
  temp.ieee.mantissa0 = (xsubi[1] << 4) | (xsubi[0] >> 28);
  temp.ieee.mantissa1 = ((xsubi[0] & 0xfffffff) << 4);
#else
# error Unsupported size of short int
#endif

  /* Please note the lower 4 bits of mantissa1 are always 0.  */
  *result = temp.d - 1.0;

  return 0;
}

double
erand48 (xsubi)
     unsigned short int xsubi[3];
{
  double result;

  (void) __erand48_r (xsubi, &__libc_drand48_data, &result);

  return result;
}

/* -------------------------- nrand48.c ---------------------------- */

static int
__nrand48_r (xsubi, buffer, result)
     unsigned short int xsubi[3];
     struct drand48_data *buffer;
     long int *result;
{
  /* Compute next state.  */
  if (__drand48_iterate (xsubi, buffer) < 0)
    return -1;

  /* Store the result.  */
  if (sizeof (unsigned short int) == 2)
    *result = xsubi[2] << 15 | xsubi[1] >> 1;
  else
    *result = xsubi[2] >> 1;

  return 0;
}

long int
nrand48 (xsubi)
     unsigned short int xsubi[3];
{
  long int result;

  (void) __nrand48_r (xsubi, &__libc_drand48_data, &result);

  return result;
}


/* -------------------------- srand48.c ---------------------------- */

static int
__srand48_r (seedval, buffer)
     long int seedval;
     struct drand48_data *buffer;
{
  /* The standards say we only have 32 bits.  */
  if (sizeof (long int) > 4)
    seedval &= 0xffffffffl;

#if USHRT_MAX == 0xffffU
  buffer->x[2] = (unsigned short)(seedval >> 16);
  buffer->x[1] = (unsigned short)(seedval & 0xffffl);
  buffer->x[0] = 0x330e;

  buffer->a[2] = 0x5;
  buffer->a[1] = 0xdeec;
  buffer->a[0] = 0xe66d;
#else
  buffer->x[2] = seedval;
  buffer->x[1] = 0x330e0000UL;
  buffer->x[0] = 0;

  buffer->a[2] = 0x5deecUL;
  buffer->a[1] = 0xe66d0000UL;
  buffer->a[0] = 0;
#endif
  buffer->c = 0xb;
  buffer->init = 1;

  return 0;
}

void
srand48 (seedval)
     long seedval;
{
  (void) __srand48_r (seedval, &__libc_drand48_data);
}


/* ------------------------- gettimeofday.c ---------------------------- */

#if 0
/* This function closely follows the approach taken by:

   GLIB - Library of useful routines for C programming
   Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
  
   gmain.c: Main loop abstraction, timeouts, and idle functions
   Copyright 1998 Owen Taylor
*/

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    static DWORD	start_tick = 0;
    static time_t	start_time;

    DWORD		tick;

    if(start_tick == 0) {
	start_tick = GetTickCount();
	time(&start_time);
    }

    tick	= GetTickCount();

    tv->tv_sec	=  (tick - start_tick) / 1000 + start_time;
    tv->tv_usec	= ((tick - start_tick) % 1000) * 1000;
    return(0);
}
#endif


/*  From Robin Rowe, robin.rowe@movieeditor.com
    Linux Journal, January 2002, p39.
 */

#include <sys/timeb.h>
#include <sys/types.h>
#include <winsock.h>

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    struct _timeb	timebuf;

    _ftime(&timebuf);
    tv->tv_sec	= timebuf.time;
    tv->tv_usec	= timebuf.millitm * 1000;
    return 0;
}

#endif

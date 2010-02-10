#include "cnetheader.h"
#include <math.h>

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

/*
    Here we'd like a fast Poisson distribution function to support the MTBF
    of nodes and links and for AL message generation rates.  Expected values
    are presented in microseconds and the actual Poisson failure variates
    returned as microseconds:

	poisson_usecs(microseconds)	-> microseconds

    Calculate random interarrival times for a Poisson process.  These
    have an exponential density             1/lambda * exp(- t/lambda),
    where lambda is the mean interarrival time.  The cumulative distribution
    function for this is                  y = 1 - exp(- t/lambda).
    The inverse of this is                t = -lambda ln(1 - y).
    According to Knuth, Vol 2, pp116, 128, we can plug a uniform random
    variable in for y, and t will have the exponential distribution we want.
    The following adjustments have been made:

    1) Even though y and 1-y have the same uniform distribution, we
       don't try to save the subtraction by substituting y for 1-y.
       This is so that the parameter to ln is never zero.  Our random
       variable y is such that 0 <= y < 1, so 0 < 1 - y <= 1.
    2) We never want a zero value.  This is achieved by subtracting
       one from the desired mean at the beginning.  This way we get
       an exponential distribution with mean lambda-1.  Then we add
       one to this random variable just before returning it.  This
       gives us a mean of lambda, and we never have a zero value.
    3) We save a negation by writing (1 - lambda) instead of -(lambda - 1).


    Thanks to Jimmy Wilkinson <wilkins@CS.cofc.EDU> for providing this code.
*/


CnetInt64 poisson_usecs(CnetInt64 mean_usecs, unsigned short *xsubi)
{
    CnetInt64	result;
    double	f1;

    int64_L2F(f1, mean_usecs);
	f1	= (((1.0 - f1) * log(1.0 - erand48(xsubi)) + 1.0));
    int64_F2L(result, f1);
    return(result);
}

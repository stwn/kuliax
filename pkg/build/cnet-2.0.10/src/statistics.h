
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


/*  These enumerated values must be in this order. */

typedef enum {
	STATS_TIME		= 0,
	STATS_EVENTS,
	STATS_MSGCREATED,
	STATS_MSGDELIVERED,
	STATS_MSGERROR,
	STATS_MSGEFFICIENCY,
	STATS_MSGRATE,
	STATS_TIMEDELIVERY,
	STATS_FRAMESTX,
	STATS_FRAMESRX,
	STATS_FRAMESCORRUPT,
	STATS_FRAMESLOST,
	STATS_FRAMECOST
} STATS_TYPE;

extern void inc_linkstats(int, int, int);
extern void inc_mainstats(STATS_TYPE, int, int);
extern void inc_nodestats(int, int);

#if	defined(USE_TCLTK)

extern void flush_allstats(ClientData);

#endif	/* defined(USE_TCLTK) */

#include "cnetheader.h"
#include "statistics.h"

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

#if	defined(USE_ASCII)

	/* nothing in this file for ASCII-only code */

/* ---------------------------------------------------------------------- */

#elif	defined(USE_TCLTK)

static	int		load_displayed	= FALSE;

static	CnetInt64	EV_COUNT_WAS[N_CNET_EVENTS];
static	CnetInt64	msgs_then, frames_then;

void flush_loadstats(CnetInt64 msgs_now, CnetInt64 frames_now)
{
    if(load_displayed) {

	CnetInt64	total, tmp64;
	int		n;
	char		cmdbuf[BUFSIZ], *p;

	total	= int64_ZERO;
	for(n=0 ; n<N_CNET_EVENTS ; ++n) {
	    int64_SUB(tmp64, EV_COUNT[n], EV_COUNT_WAS[n]);
	    int64_ADD(total, total, tmp64);
	}

	sprintf(cmdbuf, "UpdateLoad %s", int64_L2A(total,0));
	p	= cmdbuf;
	while(*p)
	    ++p;
	*p++	= ' ';
	*p++	= '{';
	for(n=0 ; n<N_CNET_EVENTS ; ++n) {
	    int64_SUB(tmp64, EV_COUNT[n], EV_COUNT_WAS[n]);
	    *p++ = ' ';
	    strcpy(p, int64_L2A(tmp64,0));
	    while(*p)
		++p;
	}
	*p++	= '}';
	*p	= '\0';
	TCLTK(cmdbuf);
    }
    memcpy(EV_COUNT_WAS, EV_COUNT, sizeof(EV_COUNT_WAS));

    frames_then	= frames_now;
    msgs_then	= msgs_now;
}

void init_loadwindow()
{
    int	n;

    msgs_then	= int64_ZERO;
    frames_then	= int64_ZERO;

    for(n=0 ; n<N_CNET_EVENTS ; ++n) {
	EV_COUNT_WAS[n]	= int64_ZERO;
	sprintf(chararray, "cnet_evname(%d)", n);
	Tcl_SetVar(tcl_interp,chararray,cnet_evname[n],TCL_GLOBAL_ONLY);
    }

    Tcl_LinkVar(tcl_interp, "load_displayed",
				(char *)&load_displayed, TCL_LINK_BOOLEAN);
}
#endif	/* defined(USE_TCLTK) */

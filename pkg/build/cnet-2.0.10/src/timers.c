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


/* --------------------------- Timer Layer ------------------------------- */


void reboot_timer_layer()
{
    NP[THISNODE].nexttimer	= 1;
}


CnetTimer CNET_start_timer(CnetEvent ev, CnetInt64 usec, CnetData data)
{
    CnetTimer	timer;

    if(int64_CMP(usec, <=, int64_ZERO) || !IS_TIMER(ev)) {
	if(gattr.trace_events)
	   TRACE("\tCNET_start_timer(ev=%d,usec=%s,data=%ld) = NULLTIMER %s\n",
			(int)ev, int64_L2A(usec,0),
			data, cnet_errname[(int)ER_BADARG]);
	cnet_errno = ER_BADARG;
	return NULLTIMER;
    }

    timer	= (CnetTimer)NP[THISNODE].nexttimer++ ;
    schedule_event(ev, THISNODE, usec, timer, data);

    if(gattr.trace_events)
	TRACE("\tCNET_start_timer(%s,usec=%s,data=%ld) = %ld\n",
			cnet_evname[(int)ev], int64_L2A(usec,0), data, timer);
    return(timer);
}


int CNET_stop_timer(CnetTimer timer)
{
    extern int	unschedule_timer(CnetTimer timer, CnetData *data);

    if(unschedule_timer(timer, (CnetData *)NULL) == TRUE) {
	if(gattr.trace_events)
	    TRACE("\tCNET_stop_timer(t=%u) = 0\n", (unsigned)timer);
	return(0);
    }

    if(gattr.trace_events)
	TRACE("\tCNET_stop_timer(t=%u) = -1 %s\n",
			    (unsigned)timer, cnet_errname[(int)ER_BADTIMER]);
    cnet_errno = ER_BADTIMER;
    return(-1);
}


int CNET_timer_data(CnetTimer timer, CnetData *data)
{
    extern int	unschedule_timer(CnetTimer timer, CnetData *data);

    if(VALID_INTPTR(data) && unschedule_timer(timer, data) == TRUE) {
	if(gattr.trace_events)
	    TRACE("\tCNET_timer_data(t=%u,%s) = 0\n",
			(unsigned)timer, find_trace_name(data));
	return(0);
    }

    cnet_errno = VALID_INTPTR(data) ? ER_BADTIMER : ER_BADARG;
    if(gattr.trace_events)
	TRACE("\tCNET_timer_data(t=%u,%s) = -1 %s\n",
			(unsigned)timer,
			find_trace_name(data),
			cnet_errname[(int)cnet_errno]);
    return(-1);
}

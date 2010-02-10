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

char *cnet_evname[] = {
    "EV_NULL",			"EV_REBOOT",		"EV_SHUTDOWN",
    "EV_APPLICATIONREADY",	"EV_PHYSICALREADY",	"EV_KEYBOARDREADY",
    "EV_LINKSTATE",		"EV_DRAWFRAME",

    "EV_DEBUG1", "EV_DEBUG2", "EV_DEBUG3", "EV_DEBUG4", "EV_DEBUG5",

    "EV_TIMER1", "EV_TIMER2", "EV_TIMER3", "EV_TIMER4", "EV_TIMER5",
    "EV_TIMER6", "EV_TIMER7", "EV_TIMER8", "EV_TIMER9", "EV_TIMER10"
};


static int check_handler(CnetEvent ev, long handler, CnetData data, char *name)
{
    if((int)ev < (int)EV_NULL || (int)ev >= N_CNET_EVENTS) {
	if(gattr.trace_events)
	  TRACE("\t%s(ev=%d,%s,data=%ld) = -1 ER_BADEVENT\n",
	    name, (int)ev, find_trace_name((void *)handler), data);
	cnet_errno = ER_BADEVENT;
	return (-1);
    }
    if(gattr.trace_events)
	TRACE("\t%s(%s,%s,data=%ld) = ",
	    name, cnet_evname[(int)ev], find_trace_name((void *)handler), data);

    if( NP[THISNODE].nodetype != NT_HOST	&&
	(ev == EV_APPLICATIONREADY || ev == EV_KEYBOARDREADY)) {
	if(gattr.trace_events)
	    TRACE("-1 ER_NOTSUPPORTED\n");
	cnet_errno = ER_NOTSUPPORTED;
	return (-1);
    }
    return 0;
}

int CNET_set_handler(CnetEvent ev, void (*handler)(), CnetData data)
{
    if(check_handler(ev, (long)handler, (long)data, "CNET_set_handler") != 0)
	return(-1);

    NP[THISNODE].handler[(int)ev - (int)EV_NULL] = handler;
    NP[THISNODE].data   [(int)ev - (int)EV_NULL] = data;
    if(gattr.trace_events)
	TRACE("0\n");
    return 0;
}

int CNET_get_handler(CnetEvent ev, void (**handler)(), CnetData *data)
{
    if(check_handler(ev, (long)handler, (long)data, "CNET_get_handler") != 0)
	return(-1);

    if(handler)
	*handler	= NP[THISNODE].handler[(int)ev - (int)EV_NULL];
    if(data)
	*data		= NP[THISNODE].data   [(int)ev - (int)EV_NULL];
    if(gattr.trace_events)
	TRACE("0\n");
    return 0;
}

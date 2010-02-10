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


/* ------------------------- Run-time errors -------------------------- */


char *cnet_errname[N_CNET_ERRORS] = {
    "ER_OK",		"ER_BADARG",		"ER_BADEVENT",
    "ER_BADLINK",	"ER_BADNODE",		"ER_BADSENDER",
    "ER_BADSESSION",	"ER_BADSIZE",		"ER_BADTIMER",
    "ER_CORRUPTDATA",	"ER_LINKDOWN",		"ER_NOTFORME",
    "ER_NOTREADY",	"ER_NOTSUPPORTED",	"ER_OUTOFSEQ",
    "ER_TOOBUSY"
};

char *cnet_errstr[N_CNET_ERRORS] = {
    /* ER_OK */		  "No error",
    /* ER_BADARG */	  "Invalid argument passed to a function",
    /* ER_BADEVENT */	  "Invalid event passed to a function",
    /* ER_BADLINK */	  "Invalid link number passed to a function",
    /* ER_BADNODE */	  "Invalid node passed to a function",
    /* ER_BADSENDER */	  "Application Layer given msg from an unknown node",
    /* ER_BADSESSION */	  "Application Layer given msg from incorrect session",
    /* ER_BADSIZE */	  "Indicated length is of incorrect size",
    /* ER_BADTIMER */	  "Invalid CnetTimer passed to a function",
    /* ER_CORRUPTDATA */  "Attempted to transfer corrupt data",
    /* ER_LINKDOWN */	  "Attempted to transmit on a link that is down",
    /* ER_NOTFORME */	  "Application Layer given msg for another node",
    /* ER_NOTREADY */	  "Function called when service not available",
    /* ER_NOTSUPPORTED */ "Invalid operation for this node type",
    /* ER_OUTOFSEQ */	  "Application Layer given msg out of sequence",
    /* ER_TOOBUSY */	  "Function is too busy/congested to handle request"
};

void CNET_perror(char *user_msg)
{
    if((int)cnet_errno < 0 || (int)cnet_errno >= N_CNET_ERRORS)
	return;
    if(user_msg == (char *)NULL || *user_msg == '\0')
	user_msg	= NP[THISNODE].nodename;
    fprintf(stderr,"%s: %s (%s)\n",
	user_msg, cnet_errname[(int)cnet_errno], cnet_errstr[(int)cnet_errno]);
}

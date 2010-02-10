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

NODEATTR DEFAULTNODE;
LINKATTR DEFAULTLINK;

GLOBALATTR gattr = {
	NULL,				/* bgimage */
	FALSE,				/* drawframes */
	FALSE,				/* showcostperbyte */
	FALSE,				/* showcostperframe */
	FALSE,				/* stdio_quiet */
	FALSE,				/* trace_events */
	(char *)NULL,			/* trace filenm */
	(FILE *)NULL			/* trace file pointer */
};

int	dflag		= FALSE;	/* debugging/diagnostics */
int	vflag		= FALSE;	/* be verbose about cnet's actions */

#if	defined(USE_TCLTK)
int	Wflag		= TRUE;		/* run under the windowing env. */
#else
int	Wflag		= FALSE;	/* run without the windowing env. */
#endif

char	*argv0;


/* ------------------- NETWORKING/TOPOLOGY VARIABLES --------------------- */


int             THISNODE	= 0;
CnetEvent	HANDLING	= EV_NULL;

int             _NNODES		= 0;
int             _NLINKS		= 0;
int             NNODES		= 0;	/* divulged with  cnet -N */
CnetInt64	EV_COUNT[N_CNET_EVENTS];

NODE            *NP;
LINK            *LP;
CnetLinkinfo	*linkinfo;
CnetNodeinfo	nodeinfo;

CnetError	cnet_errno	= ER_OK;
RUNSTATE	cnet_state	= STATE_PAUSED;

CnetInt64	TIMENOW_in_USEC;

#if	!HAVE_LONG_LONG
CnetInt64	DEFAULT64;
CnetInt64	MILLION64;
#endif


/* ----------------------- TCL/TK GLOBAL VARIABLES ----------------------- */

#if	defined(USE_TCLTK)
Display		*display;

Tcl_Interp	*tcl_interp;
Tk_Window	tcl_mainwindow;
#endif	/* defined(USE_TCLTK) */


/* ------------------------ PARSING VARIABLES ---------------------------- */

INPUT	input;
int	nerrors		= 0;

char	chararray[BUFSIZ];


/* ----------------------------------------------------------------------- */

void init_globals()
{
    int	n;

#if	!HAVE_LONG_LONG
    int64_I2L(DEFAULT64,	-1);
    int64_I2L(MILLION64,	1000000);
#endif

    DEFAULTNODE.address			= (CnetAddr)0;
    DEFAULTNODE.minmessagesize		= 100;
    DEFAULTNODE.maxmessagesize		= 8*K;

    DEFAULTNODE.messagerate		= MILLION64;
    DEFAULTNODE.nodemtbf		= int64_ZERO;
    DEFAULTNODE.nodemttr		= int64_ZERO;

    DEFAULTNODE.outputfile		= (char *)NULL;
    DEFAULTNODE.reboot_args		= (char **)NULL;
    DEFAULTNODE.reboot_func		= DEFAULT_REBOOT_FUNCTION;
    DEFAULTNODE.compile			= DEFAULT_COMPILE_STRING;
    DEFAULTNODE.osname			= (char *)NULL;
    DEFAULTNODE.stdio_quiet		= FALSE;
    DEFAULTNODE.trace_all		= FALSE;
    DEFAULTNODE.trace_mask		= 0;
    DEFAULTNODE.x			= 10;
    DEFAULTNODE.y			= 10;
    DEFAULTNODE.winx			= 10;
    DEFAULTNODE.winy			= 250;
    DEFAULTNODE.winopen			= FALSE;

    DEFAULTLINK.bandwidth		= 100*1000*1000;
    DEFAULTLINK.costperbyte		= 0;
    DEFAULTLINK.costperframe		= 0;
    DEFAULTLINK.transmitbufsize		= MAX_MESSAGE_SIZE + 1*K;
    DEFAULTLINK.probframecorrupt	= 0;
    DEFAULTLINK.probframeloss		= 0;

    DEFAULTLINK.propagationdelay	= MILLION64;
    DEFAULTLINK.linkmtbf		= int64_ZERO;
    DEFAULTLINK.linkmttr		= int64_ZERO;
    DEFAULTLINK.nextfree		= int64_ZERO;

    for(n=0 ; n<N_CNET_EVENTS ; ++n)
	EV_COUNT[n]			= int64_ZERO;
    TIMENOW_in_USEC			= int64_ZERO;
}

/* --------------------------------------------------------------------- */


char *findenv(const char *name, char *fallback)
{
    char	*value;

    value	= getenv(name);
    if(value == (char *)NULL || *value == '\0')
	value = fallback;
    if(vflag && value)
	fprintf(stderr,"%s=\"%s\"\n", name,value);
    return(value);
}

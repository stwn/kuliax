#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>

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

#define	CNET_AUTHOR		"Chris McDonald"
#define	CNET_EMAIL		"chris@csse.uwa.edu.au"

#if	!defined(FALSE)
#define	FALSE			0
#define	TRUE			1
#endif

#include "config.h"		/* installation dependent definitions */
#include "hidenames.h"		/* renames important global symbols */
#include "externs.h"		/* external function prototypes */
#include "cnet.h"		/* this is the students' header file */

#if	defined(USE_WIN32)
#include <io.h>
#include <wtypes.h>

#define	strcasecmp		 _stricmp

#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#endif


#if	defined(USE_LINUX)
#define	NDATASEGS		2
#define	APPEND_DOT_TO_LDPATH
#define	PREPEND_DOT_TO_SO_SOFILE

#elif	defined(USE_OSF1)
#define	NDATASEGS		4
#define	PREPEND_DOT_TO_SO_SOFILE
#define	UNLINK_SO_LOCATIONS

#elif	defined(USE_SUNOS)
#define	NDATASEGS		1

#elif	defined(USE_SOLARIS)
#define	NDATASEGS		1
#define	APPEND_DOT_TO_LDPATH
#define	PREPEND_DOT_TO_SO_SOFILE

#elif	defined(USE_IRIX5)
#define	NDATASEGS		2
#define	APPEND_DOT_TO_LDPATH
#define	PREPEND_DOT_TO_SO_SOFILE
#define	UNLINK_SO_LOCATIONS

#elif	defined(USE_FREEBSD)
#define	NDATASEGS		1
#define	PREPEND_DOT_TO_SO_SOFILE

#elif	defined(USE_NETBSD)
#define	NDATASEGS		2
#define	APPEND_DOT_TO_LDPATH
#define	PREPEND_DOT_TO_SO_SOFILE
 

#elif	defined(USE_WIN32)
#define	NDATASEGS		1

#endif


#if	defined(USE_ASCII)

#elif	defined(USE_TCLTK)

#include <tcl.h>
#include <tk.h>
#include "tcltk_notifier.h"

#if	(TCL_MAJOR_VERSION == 7) && (TCL_MINOR_VERSION < 5)
#define	USE_BEFORE_TCL75
#define	DoOneEvent	Tk_DoOneEvent
#define	DoWhenIdle	Tk_DoWhenIdle
#define	ALL_EVENTS	TK_ALL_EVENTS
#define	DONT_WAIT	TK_DONT_WAIT

#else
#define	DoOneEvent	Tcl_DoOneEvent
#define	DoWhenIdle	Tcl_DoWhenIdle
#define	ALL_EVENTS	TCL_ALL_EVENTS
#define	DONT_WAIT	TCL_DONT_WAIT
#endif

#define TCLTK_createcommand(str, func) \
	    Tcl_CreateCommand(tcl_interp, (char *)str, (Tcl_CmdProc *)func, \
			    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)

extern	void		TCLTK(const char *fmt, ...);
extern	void		draw_node_icon(int, int);

extern	Tcl_Interp	*tcl_interp;
extern	Tk_Window	tcl_mainwindow;

#else
#error	You have not specified a valid windowing system in config.h
#endif


/* --------------------------------------------------------------------- */

#define	NOBODY			(-1)
#define	UNKNOWN			(-1)
#define	DEFAULT			(-1)

#define	AN_ETHERNET		1000000

#if	HAVE_LONG_LONG
#define	DEFAULT64		(-1)
#define	MILLION64		1000000
#else
extern	CnetInt64		DEFAULT64;
extern	CnetInt64		MILLION64;
#endif

#define	DEF_NODE_X		50
#define	DEF_NODE_Y		50

#if  __STDC__
#define int32_MAXINT		((int)~(1U << 31))
#else
#define int32_MAXINT		((int)~((unsigned)1 << 31))
#endif


#define	WHICH(current,def)	((current == DEFAULT) ? def : current)
#define	WHICH64(current,def)	(int64_EQ(current,DEFAULT64) ? def : current)

#define	BOOL			char

typedef	enum {
    STATE_RUNNING = 0,	STATE_UNKNOWN,		STATE_REBOOTING,
    STATE_CRASHED,	STATE_AUTOREBOOT,	STATE_PAUSED,
    STATE_UNDERREPAIR,	STATE_SINGLESTEP,	STATE_TIMESUP
} RUNSTATE;

extern void schedule_event(CnetEvent ne, int node, CnetInt64 relative_usecs,
			   CnetTimer timer, CnetData data);


/* ------------------ NETWORKING/TOPOLOGY INFORMATION ------------------ */


typedef int PROBABILITY;

typedef struct {
    CnetNicaddr		nicaddr;
    int			promiscuous;
    int			bandwidth;
    int			costperbyte;
    int			costperframe;
    int			transmitbufsize;

    PROBABILITY		probframecorrupt;
    PROBABILITY		probframeloss;
    CnetInt64		propagationdelay;
    CnetInt64		linkmtbf;
    CnetInt64		linkmttr;
    CnetInt64		nextfree;
} LINKATTR;

#if	defined(USE_TCLTK)
typedef struct {
    int		displayed;
    CnetInt64	stats_count[6];
    int		stats_changed;
} LINKPANEL;
#endif

typedef struct {
    CnetLinktype linktype;
    int		 linkup;
    CnetInt64	 ntxed;
    int		 endmin,	endmax;
    LINKATTR	 lattrmin,	lattrmax;
#if	defined(USE_TCLTK)
    LINKPANEL	 panelmin,	panelmax;
#endif
} LINK;

#if	defined(USE_TCLTK)
typedef	struct {
    LINK	*link;
    LINKATTR	*la;
    LINKPANEL	*lp;
    int		from;
    int		to;
    int		n;
    int		value;
} LINKARG;
#endif


typedef struct {
    CnetAddr	address;
    int		minmessagesize;
    int		maxmessagesize;
    CnetInt64	messagerate;
    CnetInt64	nodemtbf;
    CnetInt64	nodemttr;
    char	*outputfile;
    char	**reboot_args;
    char	*reboot_func;
    char	*compile;
    char	*osname;
    int		stdio_quiet;
    int		trace_all;
    int		trace_mask;
    int		x, y;
    int		winx, winy;
    int		winopen;
} NODEATTR;


typedef struct {
    RUNSTATE		runstate;
    CnetNodetype	nodetype;
    char		*nodename;
    int			nlinks;
    int			*links;

    int			os_errno;
    CnetError		cnet_errno;

    unsigned long	length_data[NDATASEGS];
    char		*incore_data[NDATASEGS];
    char		*private_data[NDATASEGS];
    char		*original_data[NDATASEGS];

    CnetInt64		will_resume;
    CnetInt64		clock_skew;

    LINKATTR		lattr;
    NODEATTR		nattr;
    int			nexttimer;

    void		(*handler[N_CNET_EVENTS])();
    CnetData		data[N_CNET_EVENTS];
    int			outputfd;	/* to file given by -o */

    CnetInt64		stats_count[6];
    int			stats_changed;

#if	defined(USE_TCLTK)
    int			displayed;
    char		*debug_str[N_CNET_DEBUGS];
    char		*inputline;	/* from the keyboard */
    int			inputlen;
#endif
} NODE;

extern	NODE		*NP;
extern	LINK		*LP;

extern	NODEATTR	DEFAULTNODE;
extern	LINKATTR	DEFAULTLINK;


/* ------------------------ ETHERNET INFORMATION ----------------------- */


#define	ETH_SLOTTIME	52		/* microseconds */
#define	ETH_JAMTIME	52		/* microseconds */
#define	ETH_MAXBACKOFF	10		/* count */
#define	ETH_Mbps	10		/* million-bits / second */

typedef enum	{ IDLE, SENSING, ATTEMPT, TX, BACKOFF }	ETHERSTATUS;

typedef struct _eq {
    struct _eq		*next;
    ETHERSTATUS		status;		/* status of this ether-event */
    CnetInt64		when;		/* in microsconds */
    int			srcnic;		/* index into ep->nics[] */
#if	defined(USE_TCLTK)
    int			srclink;
#endif
    char		*packet;
    int			len;
} ETHERNETQUEUE;

typedef struct {
    ETHERSTATUS		status;		/* status of this NIC on this segment */
    CnetNicaddr		nicaddr;
#if ETHERNICS_CAN_BUFFER
    ETHERNETQUEUE	*head;		/* queue of this NIC's outgoing pkts */
    ETHERNETQUEUE	*tail;
#endif
    int			whichlink;	/* index into LP[] */
    int			whichnode;	/* index into NP[] */
    int			nodeslink;	/* link of NP[whichnode] */
    int			backoff;	/* 0, 1, .... MAXBACKOFF */
    int			ncollisions;
    int			nsuccesses;
} ETHERNIC;

typedef struct {
    char		*name;
    int			x, y;
    ETHERSTATUS		status;		/* this segment - IDLE | ATTEMPT | TX */
    ETHERNETQUEUE	EQ;

    int			nnics;
    ETHERNIC		*nics;
    int			nattempting;	/* #nodes in ATTEMPT */

    CnetInt64		volume;		/* in bytes */
    int			ncollisions;
    int			nsuccesses;
#if	defined(USE_TCLTK)
    LINKPANEL		panel;
#endif
} ETHERNET;

extern	ETHERNET	*ethernets;
extern	int		nethernets;


/* ------------------------------------------------------------------ */

typedef struct {
    char		*bgimage;
    int			drawframes;
    int			showcostperbyte;
    int			showcostperframe;
    int			stdio_quiet;
    int			trace_events;
    char		*trace_filenm;
    FILE		*tfp;
} GLOBALATTR;

extern	GLOBALATTR	gattr;

typedef struct {
    CnetDrawFrame	cdf;
    int			srcnode;
    CnetInt64		started;
    CnetInt64		inflight;
    CnetInt64		arrives;
    CnetInt64		when_corrupt;
    CnetInt64		when_lost;
    int			x;
} DRAWFRAME;


extern	int		THISNODE;
extern	CnetEvent	HANDLING;
extern	int		_NNODES;	/* used internally by cnet */
extern	int		_NLINKS;	/* used internally by cnet */
extern	CnetInt64	EV_COUNT[];


/* ------------- parsing information and global things -------------- */

typedef struct {
    FILE	*fp;
    char	*name;
    char	line[BUFSIZ];
    int		cc, ll, lc;

    int		value;
    CnetNicaddr	nicaddr;
} INPUT ;

extern	INPUT		input;

extern	char		chararray[];
extern	int		nerrors;

extern	RUNSTATE	cnet_state;

extern	CnetInt64	TIMENOW_in_USEC;

extern	char		*argv0;

extern	int		dflag;		/* turn on debugging/diagnostics */
extern	int		Wflag;		/* run under the windowing env. */
extern	int		vflag;		/* be verbose about cnet's actions */


extern	void		cleanup(int);
extern	char		*findenv(const char *, char *);
extern	char		*find_cnetfile(char *filenm, int wantdir, int fatal);
extern	char		*find_trace_name(void *addr);
extern	void		TRACE(const char *, ...);


/* ----------------------- some general C habits ----------------------- */

#define NEW(type)	(type *)malloc(sizeof(type))

#define	VALID_INTPTR(x)	((int *)x != (int *)NULL && ((long)x % sizeof(int))==0)

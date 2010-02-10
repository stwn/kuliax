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


#define	POLL_FREQ		10000		/* in usecs */
#define	SLEEP_PERIOD		1000		/* in usecs */
#define	PING_TCLTK_FREQ		20000		/* in usecs */

#define	SCHEDULER_RAND48_SEED	2675255

#define	CONTEXT_SWITCH()	if(THISNODE != SWAPPED_IN) context_switch()

#if	defined(USE_WIN32)
struct timezone {
    int  tz_minuteswest;			/* minutes W of Greenwich */
    int  tz_dsttime;				/* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

static	NODE			*np;		/* always &NP[THISNODE] */

static	CnetInt64		STARTUSEC;
static	CnetInt64		poll_freq;
static	CnetInt64		report_freq;
static	CnetInt64		time_finish;

static	int			events_handled, events_left;
static	int			cntx=0, SWAPPED_IN;
static	int			tell_nnodes		= 0;

static	unsigned short		xsubi[3]		= {0,0,0};

#if	defined(USE_TCLTK)
extern	void draw_link(int, int, int);
extern	void draw_map(int, int);

#define	DRAW_NODE_ICON()	if(Wflag) draw_node_icon(FALSE,THISNODE)
#define	DRAW_LINK(l)		if(Wflag) { \
				    draw_link(l, TRUE, CANVAS_FATLINK/2); \
				    draw_node_icon(FALSE,LP[l].endmin); \
				    draw_node_icon(FALSE,LP[l].endmax); }
#define	DRAW_MAP()		if(Wflag) draw_map(0,0)

#else	/* !defined(USE_TCLTK) */

#define	DRAW_NODE_ICON()
#define	DRAW_LINK(l)
#define	DRAW_MAP()

#endif	/* defined(USE_TCLTK) */


/* ------------------------------------------------------------------------ */


typedef enum {
    SE_NULL = 0,
    SE_APPLICATIONREADY,	SE_TIMER,
    SE_NODESTATE,		SE_LINKSTATE,
    SE_REPORT,			SE_TIMESUP,
#if	defined(USE_TCLTK)
    SE_DEBUGBUTTON,		SE_KEYBOARDREADY,
    SE_DRAWFRAME
#endif
} SCHEDULE;

typedef struct _sq {
    struct _sq		*next;
    SCHEDULE		se;
    int			node;
    CnetEvent		ne;
    CnetInt64		usec;
    CnetTimer		timer;
    CnetData		data;
} SCHEDULE_Q;


static	SCHEDULE_Q	GSQ	= { (SCHEDULE_Q *)NULL };
static	int		nqueued	= 0;


/* --------------------------------------------------------------------- */


/*  We keep the Global Schedule Queue sorted by:

	time	- so that events occur in order,
	node	- to minimize context switches,
	ne	- so that ack frames arrive before their timeouts
 */

static void insert_event(SCHEDULE se, CnetEvent	ne, int	node,
			 CnetInt64 abs_usec, CnetTimer timer, CnetData data)
{
    SCHEDULE_Q	*s	= &GSQ;
    SCHEDULE_Q	*t;

    t		= NEW(SCHEDULE_Q);
    t->se	= se;
    t->node	= node;
    t->ne	= ne;
    t->usec	= abs_usec;
    t->timer	= timer;
    t->data	= data;

    while(s->next) {
	if(int64_CMP(abs_usec, <, s->next->usec))
	    break;

	if(int64_EQ(abs_usec, s->next->usec)) {
	    if(node  < s->next->node)
		break;
	    if(node == s->next->node && (int)ne < (int)s->next->ne)
		break;
	}
	s	= s->next;
    }
    t->next	= s->next;
    s->next	= t;
    ++nqueued;
}

void schedule_event(CnetEvent ne, int node, CnetInt64 rel_usec,
		    CnetTimer timer, CnetData data)
{
    CnetInt64	tmp64;	
    SCHEDULE	se;

    if(node < 0) {		/* this event is for a link, not a node */
	se	= SE_LINKSTATE;
	node	= -node-1;
    }
    else if(IS_TIMER(ne))
	se	= SE_TIMER;
#if	defined(USE_TCLTK)
    else if(ne == EV_DRAWFRAME)
	se	= SE_DRAWFRAME;
    else if(IS_DEBUG(ne))
	se	= SE_DEBUGBUTTON;
    else if(ne == EV_KEYBOARDREADY)
	se	= SE_KEYBOARDREADY;
#endif
    else if(ne == EV_REBOOT)
	se	= SE_NODESTATE;
    else
	return;

    if(int64_IS_ZERO(rel_usec)) {
	int64_ADD(tmp64, TIMENOW_in_USEC, int64_ONE);
    }
    else {
	int64_ADD(tmp64, TIMENOW_in_USEC, rel_usec);
    }
    insert_event(se,ne, node, tmp64, timer, data);
}

int unschedule_timer(CnetTimer timer, CnetData *datawas)
{					/* reqd by: CNET_stop_timer() */
    SCHEDULE_Q	*t;
    SCHEDULE_Q	*s	= &GSQ;

    while((t = s->next)) {
	if(timer == t->timer && THISNODE == t->node && IS_TIMER(t->ne)) {
	    if(datawas) {
		*datawas	= t->data;
		return(TRUE);
	    }
	    s->next	= t->next;
	    free((char *)t);
	    --nqueued;
	    return(TRUE);
	}
	s	= s->next;
    }
    return(FALSE);
}

void unschedule_node(int node)		/* reqd by: node_menu.c */
{
    extern int	reboot_physical_layer(int node);

    SCHEDULE_Q	*s	= &GSQ;
    SCHEDULE_Q	*t;

    while((t = s->next)) {
	if(t->node == node && t->se != SE_LINKSTATE) {
	    s->next	= t->next;
	    free((char *)t);
	    --nqueued;
	}
	else
	    s	= s->next;
    }
    reboot_physical_layer(node);
}



/* ---------- Calculate times of node and link failure and repair --------- */


extern CnetInt64 poisson_usecs(CnetInt64, unsigned short *);

static CnetInt64 node_mtbf()
{
    CnetInt64 t1 = WHICH64(np->nattr.nodemtbf, DEFAULTNODE.nodemtbf);
    CnetInt64 t2 = WHICH64(np->nattr.nodemtbf, DEFAULTNODE.nodemtbf);
    CnetInt64 extent;

         if(int64_IS_ZERO(t1))
	extent = t2;
    else if(int64_IS_ZERO(t2) || int64_CMP(t1, <, t2))
	extent = t1;
    else
	extent = t2;

    if(int64_IS_ZERO(extent))
	return(int64_ZERO);
    else
	return(poisson_usecs(extent, xsubi));
}

static CnetInt64 node_mttr()
{
    CnetInt64 t1 = WHICH64(np->nattr.nodemttr, DEFAULTNODE.nodemttr);
    CnetInt64 t2 = WHICH64(np->nattr.nodemttr, DEFAULTNODE.nodemttr);

    if(int64_CMP(t1, >, t2))
	return(poisson_usecs(t1, xsubi));
    else
	return(poisson_usecs(t2, xsubi));
}


static CnetInt64 link_mtbf(int l)
{
    CnetInt64 t1 = WHICH64(LP[l].lattrmin.linkmtbf, DEFAULTLINK.linkmtbf);
    CnetInt64 t2 = WHICH64(LP[l].lattrmax.linkmtbf, DEFAULTLINK.linkmtbf);
    CnetInt64 extent;

         if(int64_IS_ZERO(t1))
	extent = t2;
    else if(int64_IS_ZERO(t2) || int64_CMP(t1, <, t2))
	extent = t1;
    else
	extent = t2;

    if(int64_IS_ZERO(extent))
	return(int64_ZERO);
    else
	return(poisson_usecs(extent, xsubi));
}

static CnetInt64 link_mttr(int l)
{
    CnetInt64 t1 = WHICH64(LP[l].lattrmin.linkmttr, DEFAULTLINK.linkmttr);
    CnetInt64 t2 = WHICH64(LP[l].lattrmax.linkmttr, DEFAULTLINK.linkmttr);

    if(int64_CMP(t1, >, t2))
	return(poisson_usecs(t1, xsubi));
    else
	return(poisson_usecs(t2, xsubi));
}


/* ------------------------------------------------------------------------ */


static void swapout_data(int n)
{
    NODE *np	= &NP[n];
    int	 i;

    for(i=0 ; i<NDATASEGS ; ++i)
	if(np->length_data[i])
	    memcpy(np->private_data[i], np->incore_data[i], np->length_data[i]);
}

static void swapin_data(int n)
{
    NODE *np	= &NP[n];
    int	 i;

    for(i=0 ; i<NDATASEGS ; ++i)
	if(np->length_data[i])
	    memcpy(np->incore_data[i], np->private_data[i], np->length_data[i]);
}

static void reboot_data(int n)
{
    NODE *np	= &NP[n];
    int  i;

    for(i=0 ; i<NDATASEGS ; ++i)
	if(np->length_data[i])
	    memcpy(np->incore_data[i],np->original_data[i],np->length_data[i]);
}

/* ------------------------------------------------------------------------ */

static void context_switch()
{
    CnetLinkinfo	*li;
    LINKATTR		*thislinkattr;
    int			l, whichlink;

/* swap out SWAPPED_IN process, swap in THISNODE process */

    NP[SWAPPED_IN].os_errno	= errno;
    NP[SWAPPED_IN].cnet_errno	= cnet_errno;

    swapout_data(SWAPPED_IN);
    swapin_data(THISNODE);

    nodeinfo.nodenumber	= THISNODE;
    strcpy(nodeinfo.nodename, np->nodename);
    nodeinfo.nodetype	= np->nodetype;
    nodeinfo.address	= np->nattr.address;
    nodeinfo.nlinks	= np->nlinks;

    nodeinfo.minmessagesize	=
	    WHICH(np->nattr.minmessagesize, DEFAULTNODE.minmessagesize);
    nodeinfo.maxmessagesize	=
	    WHICH(np->nattr.maxmessagesize, DEFAULTNODE.maxmessagesize);

    if(int64_EQ(np->nattr.messagerate, DEFAULT64))
	nodeinfo.messagerate	= DEFAULTNODE.messagerate;
    else
	nodeinfo.messagerate	= np->nattr.messagerate;

/*  INITIALIZE THE NODE'S LOOPBACK LINK */
    memset(&linkinfo[0], 0, sizeof(CnetLinkinfo));
    linkinfo[0].linktype	= LT_LOOPBACK;
    linkinfo[0].linkup		= TRUE;

/*  INITIALIZE THE NODE'S PHYSICAL LINKS */
    for(l=1, li=&linkinfo[1] ; l<=np->nlinks ; ++l, ++li) {
	whichlink		= np->links[l];

	if( LP[whichlink].endmin == THISNODE)
	    thislinkattr	= &(LP[whichlink].lattrmin);
	else
	    thislinkattr	= &(LP[whichlink].lattrmax);

	li->linktype		= LP[whichlink].linktype;
	li->linkup		= LP[whichlink].linkup;
	li->promiscuous		= thislinkattr->promiscuous;
	memcpy(li->nicaddr, thislinkattr->nicaddr, LEN_NICADDR);

	li->bandwidth		=
	    WHICH(thislinkattr->bandwidth, DEFAULTLINK.bandwidth);
	li->transmitbufsize	=
	    WHICH(thislinkattr->transmitbufsize, DEFAULTLINK.transmitbufsize);

	li->costperbyte		=
	    WHICH(thislinkattr->costperbyte, DEFAULTLINK.costperbyte);
	li->costperframe	=
	    WHICH(thislinkattr->costperframe, DEFAULTLINK.costperframe);

	if(int64_EQ(thislinkattr->propagationdelay, DEFAULT64))
	    li->propagationdelay = DEFAULTLINK.propagationdelay;
	else
	    li->propagationdelay = thislinkattr->propagationdelay;
    }

    errno		= np->os_errno;
    cnet_errno		= np->cnet_errno;
    NNODES		= tell_nnodes;
    SWAPPED_IN		= THISNODE;
    cntx++;
}

/* ------------------------------------------------------------------------ */


static void HANDLER(CnetEvent ev, CnetTimer timer, CnetData data)
{
    extern void		clear_trace_names(void);
    CnetInt64		usec, tmp64;

    int64_ADD(usec, STARTUSEC, TIMENOW_in_USEC);
    int64_ADD(usec, usec, np->clock_skew);
    int64_DIV(tmp64, usec, MILLION64);
    int64_L2I(nodeinfo.time_of_day.sec, tmp64);

    int64_MOD(tmp64, usec, MILLION64);
    int64_L2I(nodeinfo.time_of_day.usec, tmp64);

    nodeinfo.time_in_usec	 = TIMENOW_in_USEC;

    HANDLING = ev;
    int64_ADD(EV_COUNT[(int)HANDLING], EV_COUNT[(int)HANDLING], int64_ONE);
    inc_mainstats(STATS_EVENTS,0,0);

    gattr.stdio_quiet	= DEFAULTNODE.stdio_quiet	|
			  np->nattr.stdio_quiet;

    gattr.trace_events	= DEFAULTNODE.trace_all		|
			  np->nattr.trace_all		|
			  (np->nattr.trace_mask & (1<<(int)HANDLING));

    if(gattr.trace_events) {
	if(timer == NULLTIMER)
	    TRACE("enter %s(%s,NULLTIMER,data=%ld) @%susec\n",
			np->nodename, cnet_evname[(int)HANDLING],
			data, int64_L2A(TIMENOW_in_USEC,1));
	else
	    TRACE("enter %s(%s,t=%ld,data=%ld) @%susec\n",
			np->nodename, cnet_evname[(int)HANDLING], timer,
			data, int64_L2A(TIMENOW_in_USEC,1));
    }
    np->handler[(int)HANDLING](HANDLING, timer, data);
    clear_trace_names();
    if(gattr.trace_events)
	TRACE("leave %s(%s)\n\n", np->nodename, cnet_evname[(int)HANDLING]);

    ++events_handled;
    --events_left;
}

static void invoke_reboot()
{
    if(np->handler[(int)EV_REBOOT]) {

	extern int	reboot_application_layer(CnetInt64 *);
	extern int	reboot_physical_layer(int node);
	extern int	reboot_stdio_layer(void);
	extern void	reboot_timer_layer(void);

	CnetInt64	do_next;

	CONTEXT_SWITCH();		/* only switch out running node */
	reboot_data(THISNODE);
	unschedule_node(THISNODE); /* calls reboot_physical_layer(THISNODE) */

	if(np->nodetype == NT_HOST) {
	    int64_ADD(do_next, TIMENOW_in_USEC, poll_freq);
	    reboot_application_layer(&do_next);
	    insert_event(SE_APPLICATIONREADY, EV_APPLICATIONREADY,
				    THISNODE, do_next, NULLTIMER, 0);
	}

	reboot_stdio_layer();
	reboot_timer_layer();

	np->os_errno	= 0;
	np->cnet_errno	= ER_OK;
	cnet_errno	= ER_OK;
	HANDLER(EV_REBOOT, NULLTIMER, (CnetData)np->nattr.reboot_args);
	np->runstate	= STATE_RUNNING;

/*  NOW INSERT AN EVENT IF THIS NODE IS DOOMED TO FAIL */
	do_next	= node_mtbf();
	if(! int64_IS_ZERO(do_next)) {
	    int64_ADD(do_next, TIMENOW_in_USEC, do_next);
	    insert_event(SE_NODESTATE, EV_REBOOT,
				    THISNODE, do_next, NULLTIMER, 0);
	}
    }
    else {
	fprintf(stderr,"%s: %s rebooted without an EV_REBOOT handler!\n",
				argv0, np->nodename);
	cleanup(2);
    }
}

void invoke_shutdown(int which)
{
    np	= &(NP[THISNODE = which]);

    if(np->handler[(int)EV_SHUTDOWN]) {
	CONTEXT_SWITCH();
	HANDLER(EV_SHUTDOWN, NULLTIMER, np->data[(int)EV_SHUTDOWN]);
    }
}


/* ------------------------------------------------------------------------ */


#if	defined(USE_TCLTK)
static CnetInt64	ping_tcltk_freq;
static CnetInt64	time_next_ping;
#endif


void init_scheduler(int fflag, int Mflag, int Nflag, int Sflag)
{
/*  INIT SCHEDULER BY TAKING LOCAL COPIES OF COMMAND-LINE OPTIONS */

    int64_I2L(report_freq, fflag);
    int64_MUL(report_freq, report_freq, MILLION64);
    int64_I2L(poll_freq, POLL_FREQ);

    int64_I2L(time_finish, Mflag*60);
    int64_MUL(time_finish, time_finish, MILLION64);
    int64_ADD(time_finish, time_finish, int64_ONE);

    STARTUSEC	= int64_ZERO;

    tell_nnodes		= Nflag ? _NNODES : 0;

    Sflag += SCHEDULER_RAND48_SEED;
    memcpy((char *)xsubi, (char *)&Sflag, sizeof(Sflag));
}

/* ------------------------------------------------------------------------ */

static CnetInt64 wall_clock()
{
    struct timeval	NOW;
    CnetInt64		now_usecs, tmp64;

    gettimeofday(&NOW, (struct timezone *)NULL);
    int64_I2L(now_usecs, NOW.tv_sec);
    int64_MUL(now_usecs, now_usecs, MILLION64);
    int64_I2L(tmp64, NOW.tv_usec);
    int64_ADD(now_usecs, now_usecs, tmp64);
    return(now_usecs);
}

static void start_scheduler()
{
    CnetInt64		do_next;
    int			l, n;

    STARTUSEC		= wall_clock();
    TIMENOW_in_USEC	= int64_ZERO;
#if	defined(USE_TCLTK)
    if(Wflag) {
	int64_I2L(ping_tcltk_freq, PING_TCLTK_FREQ);
	int64_ADD(time_next_ping, STARTUSEC, ping_tcltk_freq);
    }
#endif

    THISNODE		= -1;
    SWAPPED_IN		= _NNODES-1;

/*  IT IS ESSENTIAL THAT NODE 0 REBOOTS FIRST (cf. context_switch() ) */
    for(n=0 ; n<_NNODES ; ++n) {
	NP[n].runstate = STATE_REBOOTING;
	insert_event(SE_NODESTATE, EV_REBOOT, n, int64_ZERO, NULLTIMER, 0);
    }

/*  NEXT, INSERT ALL POSSIBLE LINK FAILURES */
    for(l=0 ; l<_NLINKS ; ++l) {
	if(LP[l].linktype != LT_POINT2POINT)
	    continue;

	do_next	= link_mtbf(l);
	if(! int64_IS_ZERO(do_next))
	    insert_event(SE_LINKSTATE,EV_SHUTDOWN,l, do_next, NULLTIMER, 0);
    }

/*  FINALLY, INSERT ALL SUNDRY SCHEDULABLE EVENTS */
    insert_event(SE_TIMESUP, EV_NULL, ALLNODES, time_finish, NULLTIMER, 0);
    if(! int64_IS_ZERO(report_freq))
	insert_event(SE_REPORT, EV_NULL, ALLNODES, report_freq, NULLTIMER, 0);
}


#if	defined(USE_TCLTK)
static int PING_TCLTK()
{
    extern void		move_drawframes(void);

    CnetInt64		now_usecs;

    now_usecs	= wall_clock();
    if(int64_CMP(now_usecs, <, time_next_ping))		/* not yet ... */
	return(FALSE);

    if(gattr.drawframes)
	move_drawframes();
    tcltk_notify_dispatch();

    int64_ADD(time_next_ping, now_usecs, ping_tcltk_freq);
    return(TRUE);
}
#endif


/* ------------------------------------------------------------------------ */


int schedule(int Eflag, int Tflag)
{
    extern int		poll_physical(CnetInt64 *when, int *node);

    SCHEDULE_Q		*sq;
    SCHEDULE		se;
    CnetInt64		do_next, tmp64;
    CnetEvent		ne	= EV_NULL;
    CnetTimer		timer	= NULLTIMER;
    CnetData		data;
    int			node;
    int			physical_ready;


    if(int64_IS_ZERO(STARTUSEC))
	start_scheduler();

    events_handled	= 0;
    cntx		= 0;
    events_left		= Eflag;
    cnet_state		= STATE_RUNNING;

    while(GSQ.next && events_left) {

/*  FIRSTLY, CHECK TO SEE IF WE HAVE ANY FRAMES FOR IMMEDIATE DELIVERY.
    THIS IS A BIT MORE TRICKY/UGLY THAN I'D LIKE BECAUSE WE ARE MERGING
    THE PHYSICAL FRAME QUEUE WITH THE MAIN SCHEDULING QUEUE */

	int64_ADD(do_next, TIMENOW_in_USEC, poll_freq);
	physical_ready	= poll_physical(&do_next, &node);

	if(physical_ready == FALSE && int64_CMP(GSQ.next->usec, <, do_next))
	    do_next	= GSQ.next->usec;

/*  WE NEXT ADVANCE TIMENOW_in_USEC TO THE TIME OF THE NEXT PENDING EVENT.
    WE EITHER DO THIS IMMEDIATELY OR MORE SLOWLY VIA A SERIES OF SHORT SLEEPs.
    WE USE THIS OPPORTUNITY TO SEE IF ANY TCL/TK EVENTS NEED PROCESSING. */

	if(Tflag) {
	    TIMENOW_in_USEC = do_next;
#if	defined(USE_TCLTK)
	    if(Wflag && PING_TCLTK() && cnet_state != STATE_RUNNING)
		return(events_left);
#endif
	}
	else
	    while(int64_CMP(TIMENOW_in_USEC, <, do_next)) {
		struct timeval timeout = { 0, SLEEP_PERIOD };

		select(0,(fd_set *)NULL,(fd_set *)NULL,(fd_set *)NULL,&timeout);

		tmp64	= wall_clock();
		int64_SUB(tmp64, tmp64, STARTUSEC);
		if(int64_CMP(tmp64, >, do_next))
		     TIMENOW_in_USEC	= do_next;
		else
		     TIMENOW_in_USEC	= tmp64;
#if	defined(USE_TCLTK)
		if(Wflag && PING_TCLTK() && cnet_state != STATE_RUNNING)
		    return(events_left);
#endif
	    }

/*  A PHYSICAL LAYER FRAME IS AVAILABLE */
	if(physical_ready) {
	    THISNODE	= node;
	    np		= &(NP[THISNODE]);

	    if(np->handler[ (int)EV_PHYSICALREADY ]) {
		CONTEXT_SWITCH();
		HANDLER(EV_PHYSICALREADY, NULLTIMER,
			np->data[(int)EV_PHYSICALREADY]);
	    }
	}
	if(int64_CMP(GSQ.next->usec, >, TIMENOW_in_USEC))
	    continue;

/*  DELIVER A NORMAL EVENT FROM THE EVENT QUEUE */
	sq		= GSQ.next;
	se		= sq->se;
	ne		= sq->ne;
	timer		= sq->timer;
	data		= sq->data;
	THISNODE	= sq->node;
	GSQ.next	= sq->next;
	free((char *)sq);
	--nqueued;

	switch ((int)se) {

	case SE_APPLICATIONREADY : {
	    extern int	poll_application(CnetInt64 *);

	    np		= &(NP[THISNODE]);
	    if(np->runstate == STATE_PAUSED) {
		int64_ADD(do_next, np->will_resume, int64_ONE);
	    }
	    else {
		int64_ADD(do_next, TIMENOW_in_USEC, poll_freq);

		if(poll_application(&do_next)			&&
		   np->handler[ (int)EV_APPLICATIONREADY ]) {
		    CONTEXT_SWITCH();
		    HANDLER(EV_APPLICATIONREADY, NULLTIMER, data);
		}
	    }
	    insert_event(SE_APPLICATIONREADY, EV_APPLICATIONREADY,
			    THISNODE, do_next, NULLTIMER, 0);
	    break;
	}				/* end of SE_APPLICATIONREADY */

#if	defined(USE_TCLTK)
	case SE_DEBUGBUTTON :
	case SE_KEYBOARDREADY :
#endif
	case SE_TIMER :
	    np		= &(NP[THISNODE]);
	    if(np->runstate == STATE_PAUSED) {
		int64_ADD(do_next, np->will_resume, int64_ONE);
		insert_event(se, ne,THISNODE, do_next, NULLTIMER, 0);
	    }
	    else if(np->handler[ (int)ne ]) {
#if	defined(USE_TCLTK)
		if(se == SE_DEBUGBUTTON) {
		    int	sq1			= np->nattr.stdio_quiet;
		    int	sq2			= DEFAULTNODE.stdio_quiet;

		    np->nattr.stdio_quiet	= FALSE;
		    DEFAULTNODE.stdio_quiet	= FALSE;
			CONTEXT_SWITCH();
			HANDLER(ne, timer, data);
		    np->nattr.stdio_quiet	= sq1;
		    DEFAULTNODE.stdio_quiet	= sq2;
		}
		else {
		    CONTEXT_SWITCH();
		    HANDLER(ne, timer, data);
		}
#else
		CONTEXT_SWITCH();
		HANDLER(ne, timer, data);
#endif
	    }
	    break;

	case SE_NODESTATE : {
	    np		= &(NP[THISNODE]);
	    switch ((int)np->runstate) {

	    case STATE_AUTOREBOOT :
		invoke_shutdown(THISNODE);
		invoke_reboot();
		break;

	    case STATE_UNDERREPAIR :
		if(dflag)
		    fprintf(stderr,"%s.repaired\n",np->nodename);

	    case STATE_CRASHED :
	    case STATE_REBOOTING :
		invoke_reboot();
		break;

	    case STATE_PAUSED :
		np->runstate	= STATE_RUNNING;
		if(dflag)
		    fprintf(stderr,"%s.resumed\n",np->nodename);
		break;

	    case STATE_RUNNING :
		np->runstate	= STATE_UNDERREPAIR;
		if(dflag)
		    fprintf(stderr,"%s.underrepair\n",np->nodename);

		unschedule_node(THISNODE);
		do_next = node_mttr();
		if(! int64_IS_ZERO(do_next)) {
		    int64_ADD(do_next, do_next, TIMENOW_in_USEC);
		    insert_event(SE_NODESTATE, EV_REBOOT, THISNODE,
				 do_next, NULLTIMER, 0);
		}
		break;
	    }				/* end of switch ((int)np->runstate) */
	    DRAW_NODE_ICON();
	    break;
	}				/* end of case SE_NODESTATE */

	case SE_LINKSTATE : {
	    extern void report_linkstate(int);

	    int		l = THISNODE;	/* l is the link # that just changed */

	    if(!LP[l].linkup && ne == EV_REBOOT) {
		LP[l].linkup = TRUE;
		do_next	= link_mtbf(l);
		if(! int64_IS_ZERO(do_next)) {
		    int64_ADD(do_next, do_next, TIMENOW_in_USEC);
		    insert_event(SE_LINKSTATE, EV_SHUTDOWN, l,
				do_next, NULLTIMER, 0);
		}
	    }
	    else if(LP[l].linkup && ne == EV_SHUTDOWN) {
		extern	int	clear_physical_link(int linkno);
		extern	void	lose_all_drawframes(void);

		LP[l].linkup = FALSE;
		clear_physical_link(l);
#if	defined(USE_TCLTK)
		if(gattr.drawframes)
		    lose_all_drawframes();
#endif
		do_next	= link_mttr(l);
		if(! int64_IS_ZERO(do_next)) {
		    int64_ADD(do_next, do_next, TIMENOW_in_USEC);
		    insert_event(SE_LINKSTATE, EV_REBOOT, l,
				do_next, NULLTIMER, 0);
		}
	    }
	    else
		break;
	    if(dflag)
		fprintf(stderr,"%s->%s.%s\n",
			NP[LP[l].endmin].nodename, NP[LP[l].endmax].nodename,
			LP[l].linkup ? "up" : "down" );
	    report_linkstate(l);
	    break;
	}				/* end of SE_LINKSTATE */

#if	defined(USE_TCLTK)
	case SE_DRAWFRAME : {
	    extern void		add_drawframe(DRAWFRAME *df);
	    extern void		free_drawframe(DRAWFRAME *df);
	    DRAWFRAME		*df;

	    np		= &(NP[THISNODE]);
	    df		= (DRAWFRAME *)data;
	    if(np->handler[(int)EV_DRAWFRAME]) {
		CONTEXT_SWITCH();
		HANDLER(EV_DRAWFRAME, NULLTIMER, (CnetData)&df->cdf);
		add_drawframe(df);
	    }
	    else
		free_drawframe(df);
	    break;
	}				/* end of SE_DRAWFRAME */
#endif

	case SE_REPORT : {
	    extern void	stats_summary(void);

	    if(dflag)
		fprintf(stderr,
		    "\tusec=%s contexts=%5d events=%6d nqueued=%3d\n",
				int64_L2A(report_freq,0),
				cntx, events_handled, nqueued);
	    stats_summary();
	    DRAW_MAP();
	    events_handled	= 0;
	    cntx		= 0;
	    int64_ADD(tmp64, TIMENOW_in_USEC, report_freq);
	    insert_event(SE_REPORT, EV_NULL, ALLNODES, tmp64, NULLTIMER, 0);
	    break;
	}				/* end of SE_REPORT */

	case SE_TIMESUP :
	    int64_SUB(TIMENOW_in_USEC, TIMENOW_in_USEC, int64_ONE);
	    cnet_state	= STATE_TIMESUP;
	    return(events_left);
	}				/* end of switch( SE_? ) */
    }		/* while(GSQ.next) */
    return(events_left);
}


static void handle_linkstate(int l, int n)
{
    int	whichlink;

    np		= &(NP[n]);
    for(whichlink=1 ; whichlink<=np->nlinks ; ++whichlink)
	if(l == np->links[whichlink])
	    break;
    if(n == SWAPPED_IN)
	linkinfo[whichlink].linkup	= LP[l].linkup;
    if(np->handler[ (int)EV_LINKSTATE ]) {
	THISNODE	= n;
	CONTEXT_SWITCH();
	HANDLER(EV_LINKSTATE, NULLTIMER, whichlink);
    }
}

void report_linkstate(int l)
{
    DRAW_LINK(l);

/*  ANNOUNCE LINKSTATE CHANGE TO EACH NODE, IF THEY WANT IT */
    handle_linkstate(l, LP[l].endmin);
    handle_linkstate(l, LP[l].endmax);
}


#if	defined(USE_TCLTK)
void single_event(int n, CnetEvent ev, CnetTimer timer, CnetData data)
{
    if(NP[n].runstate == STATE_RUNNING && NP[n].handler[(int)ev])  {
	THISNODE	= n;
	np		= &(NP[THISNODE]);
	CONTEXT_SWITCH();
	HANDLER(ev, timer, data);
    }
}
#endif	/* defined(USE_TCLTK) */


/* ------------------------------------------------------------------------ */


/* DOESN'T REALLY BELONG IN THIS FILE! */

int CNET_set_time_of_day(long newsec, long newusec)
{
    CnetInt64	tmp64, delta;

    if(gattr.trace_events)
	TRACE("\tCNET_set_time_of_day(newsec=%ld, newusec=%ld) = ",
					newsec, newusec);

    if(newsec < 0 || newusec < 0 || newusec >= 1000000) {
	if(gattr.trace_events)
	    TRACE("-1 %s\n", cnet_errname[(int)ER_BADARG]);
	cnet_errno	= ER_BADARG;
	return(-1);
    }

    int64_I2L(delta, newsec);
    int64_MUL(delta, delta, MILLION64);
    int64_I2L(tmp64, newusec);
    int64_ADD(delta, delta, tmp64);

    int64_ADD(tmp64, STARTUSEC, TIMENOW_in_USEC);
    int64_SUB(delta, delta, tmp64);

    NP[THISNODE].clock_skew	= delta;

    nodeinfo.time_of_day.sec	= newsec;
    nodeinfo.time_of_day.usec	= newusec;

    if(gattr.trace_events)
	TRACE("0\n");
    return(0);
}

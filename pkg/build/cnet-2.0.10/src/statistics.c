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

static char *stats_titles[] = {
	"Simulation time",
	"Events raised",
	"Messages generated",
	"Messages delivered",
	"Errors detected",
	"Efficiency (bytes AL/PL)",
	"Average delivery rate",
	"Average delivery time",
	"Frames transmitted",
	"Frames received",
	"Frames corrupted / collisions",
	"Frames lost",
	"Total transmission cost",

	"usec",
	"%",
	"KB/s",
	"usec"
};

#define	N_STATS			13
#define	N_STATS_TITLES		(sizeof(stats_titles)/sizeof(stats_titles[0]))

#define	P(n)			n, ((n)==1 ? "" : "S")


/* ------------------ Global stats information first ------------------ */


static	int		zflag			= FALSE;

static	CnetInt64	STATS[N_STATS];
static	CnetInt64	frame_txbytes;
static	CnetInt64	msg_rxbytes;

void stats_summary()
{
    CnetInt64	s;
    int		i, r;

    for(i=0, r=0 ; i<_NNODES ; ++i)
	if(NP[i].nodetype == NT_ROUTER) r++;

    fprintf(stdout, "%d HOST%s, %d ROUTER%s and %d LINK%s\n",
			P(_NNODES-r), P(r), P(_NLINKS) );

    STATS[(int)STATS_TIME]	= TIMENOW_in_USEC;
    for(i=0 ; i<N_STATS ; ++i) {
	s	= STATS[i];
	if(int64_IS_ZERO(s) && zflag == FALSE)
	    continue;

	if(i == (int)STATS_MSGRATE) {
	    double	KBps = 0.0;

	    if(! int64_IS_ZERO(msg_rxbytes)) {
		double	e1, e2;

		int64_L2F(e1, msg_rxbytes);
		int64_L2F(e2, TIMENOW_in_USEC);
		KBps	= (e1 / 1024.0) / (e2 / 1000000.0);
	    }
	    fprintf(stdout, "%-29s: %.2f\n",
			    stats_titles[(int)STATS_MSGRATE], KBps);
	}
	else if(i == (int)STATS_MSGEFFICIENCY) {
	    double eff = 0.0;

	    if(! int64_IS_ZERO(frame_txbytes)) {
		double	e1, e2;

		int64_L2F(e1, msg_rxbytes);
		int64_L2F(e2, frame_txbytes);
		eff	= e1 / e2 * 100.0;
	    }
	    fprintf(stdout, "%-29s: %.2f\n",
		    stats_titles[(int)STATS_MSGEFFICIENCY], eff);
	}
	else {
	    if(i == (int)STATS_TIMEDELIVERY && !int64_IS_ZERO(s))
		int64_DIV(s,	STATS[(int)STATS_TIMEDELIVERY],
				STATS[(int)STATS_MSGDELIVERED] );

	    fprintf(stdout,"%-29s: %s\n", stats_titles[i], int64_L2A(s,0));
	}
    }
    for(i=0 ; i<N_CNET_EVENTS ; ++i)
	if(!int64_IS_ZERO(EV_COUNT[i]) || zflag)
	    fprintf(stdout,"%-29s: %s\n",
				cnet_evname[i],int64_L2A(EV_COUNT[i],0));
    fflush(stdout);
}


void inc_mainstats(STATS_TYPE type, int delta1, int delta2)
{
    CnetInt64	tmp64;

    int64_ADD(STATS[(int)type], STATS[(int)type], int64_ONE);

    if(type == STATS_FRAMESTX) {
	int64_ADD(STATS[(int)STATS_MSGEFFICIENCY],
		  STATS[(int)STATS_MSGEFFICIENCY], int64_ONE);

	int64_I2L(tmp64, delta1);
	int64_ADD(STATS[(int)STATS_FRAMECOST],
		  STATS[(int)STATS_FRAMECOST], tmp64);

	int64_I2L(tmp64, delta2);
	int64_ADD(frame_txbytes, frame_txbytes, tmp64);
    }
    else if(type == STATS_MSGDELIVERED) {
	int64_I2L(tmp64, delta1);
	int64_ADD(STATS[(int)STATS_TIMEDELIVERY],
		  STATS[(int)STATS_TIMEDELIVERY], tmp64);

	int64_I2L(tmp64, delta2);
	int64_ADD(msg_rxbytes, msg_rxbytes, tmp64);
    }
}


/* ----------------- TCLTK windowing-specific stats follows ---------------- */

#if	defined(USE_TCLTK)

static	int		stats_displayed		= FALSE;
static	CnetInt64	msgs_ok;
static	CnetInt64	msgs_err;
static	CnetInt64	last_sum;

void init_statswindow()
{
    int	n;

    msgs_ok	= int64_ZERO;
    msgs_err	= int64_ZERO;
    last_sum	= int64_ZERO;

    for(n=0 ; n<N_STATS_TITLES ; ++n) {
	sprintf(chararray, "STATS_TITLE(%d)", n);
	Tcl_SetVar(tcl_interp,chararray,stats_titles[n],TCL_GLOBAL_ONLY);
    }
    Tcl_SetVar(tcl_interp,"UPDATE_TITLE",UPDATE_TITLE,TCL_GLOBAL_ONLY);
    Tcl_LinkVar(tcl_interp, "stats_displayed",
				(char *)&stats_displayed, TCL_LINK_BOOLEAN);
}

static void flush_mainstats()
{
    CnetInt64		sum;

    STATS[(int)STATS_TIME]	= TIMENOW_in_USEC;
    if(stats_displayed) {

	int	i;
	char	cmdbuf[BUFSIZ];
	char	*p;

	strcpy(cmdbuf, "UpdateMainstats");
	p	= cmdbuf;
	while(*p)
	    ++p;
	*p++	= ' ';
	*p++	= '{';
	for(i=0 ; i<N_STATS ; ++i) {
	    *p++	= ' ';
	    if(i == (int)STATS_MSGRATE) {
		double	KBps = 0.0;

		if(! int64_IS_ZERO(msg_rxbytes)) {
		    double	e1, e2;

		    int64_L2F(e1, msg_rxbytes);
		    int64_L2F(e2, TIMENOW_in_USEC);
		    KBps	= (e1 / 1024.0) / (e2 / 1000000.0);
		}
		sprintf(p, "%.2f", KBps);
	    }
	    else if(i == (int)STATS_MSGEFFICIENCY) {
		if(int64_IS_ZERO(frame_txbytes)) {
		    *p++	= '-';
		    *p		= '\0';
		}
		else {
		    double	e1, e2;

		    int64_L2F(e1, msg_rxbytes);
		    int64_L2F(e2, frame_txbytes);
		    sprintf(p, "%.2f", e1 / e2 * 100.0);
		}
	    }
	    else if(i == (int)STATS_TIMEDELIVERY) {
		if(int64_IS_ZERO(STATS[(int)STATS_MSGDELIVERED])) {
		    *p++	= '-';
		    *p		= '\0';
		}
		else {
		    CnetInt64	tmp64;

		    int64_DIV(tmp64, STATS[(int)STATS_TIMEDELIVERY],
				     STATS[(int)STATS_MSGDELIVERED]);
		    strcpy(p, int64_L2A(tmp64,1));
		}
	    }
	    else if(int64_IS_ZERO(STATS[i])) {
		*p++	= '-';
		*p	= '\0';
	    }
	    else
		 strcpy(p, int64_L2A(STATS[i],1));
	    while(*p)
		++p;
	}
	*p++	= ' ';
	*p++	= '}';
	*p	= '\0';
	TCLTK(cmdbuf);
    }

    int64_ADD(sum, msgs_ok, msgs_err);
    if(int64_NE(sum, last_sum)) {
	double		ratio = 100.0;

	if(! int64_IS_ZERO(msgs_err)) {
	    double	r1, r2;

	    int64_L2F(r1, msgs_ok);
	    int64_L2F(r2, sum);
	    ratio	= r1 / r2 * 100.0;
	}
	sprintf(chararray,"%10s (%5.1f%%)", int64_L2A(msgs_ok,1), ratio);
	Tcl_SetVar(tcl_interp, "CN_DELIVERY", chararray, TCL_GLOBAL_ONLY);
	last_sum	= sum;
    }
}

/* --------------- Per-node TCL/TK stats code follows ---------------- */

void inc_nodestats(int which_stat, int bytes)
{
    NODE	*np;
    CnetInt64	tmp64;
    int		c;

/*  which_stat	= 0	=> GENERATED,
		= 1	=> RECEIVED CORRECTLY,
		= 2	=> RECEIVED IN ERROR
 */

    if(which_stat == 1)	{
	int64_ADD(msgs_ok,  msgs_ok,  int64_ONE);
    }
    else if(which_stat == 2) {
	int64_ADD(msgs_err, msgs_err, int64_ONE);
    }
    c	= which_stat*2;

    np	= &NP[THISNODE];
    int64_ADD(np->stats_count[c],   np->stats_count[c],   int64_ONE);
    int64_I2L(tmp64, bytes);
    int64_ADD(np->stats_count[c+1], np->stats_count[c+1], tmp64);
    np->stats_changed	= TRUE;
}

static void flush_nodestats()
{
    NODE	*np;
    int		n, s;

    for(n=0, np=NP ; n<_NNODES ; ++n, ++np)
	if(np->displayed && np->stats_changed && np->nodetype == NT_HOST) {
	    char	cmdbuf[BUFSIZ], *p;

	    p	= cmdbuf;
	    sprintf(p, "UpdateNodeStats %d {", n);
	    while(*p)
		++p;
	    for(s=0 ; s<6 ; ++s) {
		sprintf(p, " %s", int64_L2A(np->stats_count[s],1));
		while(*p)
		    ++p;

		if(s == 1 || s == 3) {
		    double	KBps = 0.0;

		    if(! int64_IS_ZERO(np->stats_count[s])) {
			double	e1, e2;

			int64_L2F(e1, np->stats_count[s]);
			int64_L2F(e2, TIMENOW_in_USEC);
			KBps	= (e1 / 1024.0) / (e2 / 1000000.0);
		    }
		    sprintf(p, " %.2f", KBps);
		    while(*p)
			++p;
		}
	    }
	    *p++	= '}';
	    *p		= '\0';
	    TCLTK(cmdbuf);
	    np->stats_changed	= FALSE;
	}
}

/* ------------------ Per-link TCL/TK stats code follows ------------- */

void inc_linkstats(int link, int which_stat, int bytes)
{
    CnetInt64	tmp64;
    int		c;
    LINKPANEL	*lpp;
    LINK	*lp	= &LP[link];

/*  which_stat	= 0	=> TRANSMITTED,
		= 1	=> RECEIVED,
		= 2	=> ERRORS INTRODUCED / ETHERNET COLLISIONS
 */

    if(lp->linktype == LT_ETHERNET)
	lpp	= &ethernets[lp->endmax].panel;
    else {
	if(lp->endmin == THISNODE)
	    lpp	= &lp->panelmin;
	else
	    lpp	= &lp->panelmax;
	if(which_stat == 0) {
	    int64_I2L(tmp64, bytes);
	    int64_ADD(lp->ntxed, lp->ntxed, tmp64);
	}
    }
    c		= which_stat*2;

    int64_ADD(lpp->stats_count[c],	lpp->stats_count[c],	int64_ONE);
    int64_I2L(tmp64, bytes);
    int64_ADD(lpp->stats_count[c+1],	lpp->stats_count[c+1],	tmp64);
    lpp->stats_changed	= TRUE;
}

static void flush_1_linkstat(CnetLinktype t, LINKPANEL *lpp, int from, int to)
{
    char	cmdbuf[BUFSIZ], *p;
    int		s;

    p	= cmdbuf;
    if(t == LT_ETHERNET)
	sprintf(p, "UpdateLinkStats eth %d {", from);
    else
	sprintf(p, "UpdateLinkStats %d %d {", from, to);
    while(*p)
	++p;
    for(s=0 ; s<6 ; ++s) {
	if(s == 5 && t == LT_ETHERNET)
	    strcpy(p, " -");
	else
	    sprintf(p, " %s", int64_L2A(lpp->stats_count[s],1));
	while(*p)
	    ++p;

	if(s == 1 || s == 3) {
	    double	KBps = 0.0;

	    if(! int64_IS_ZERO(lpp->stats_count[s])) {
		double	e1, e2;

		int64_L2F(e1, msg_rxbytes);
		int64_L2F(e2, TIMENOW_in_USEC);
		KBps	= (e1 / 1024.0) / (e2 / 1000000.0);
	    }
	    sprintf(p, " %.2f", KBps);
	    while(*p)
		++p;
	}
    }
    *p++	= '}';
    *p		= '\0';
    TCLTK(cmdbuf);
    lpp->stats_changed	= FALSE;
}

static void flush_linkstats()
{
    int		l;
    LINK	*lp;
    LINKPANEL	*lpp;

    for(l=0, lp=LP ; l<_NLINKS ; ++l, ++lp) {

	if(lp->linktype == LT_ETHERNET) {
	    lpp	= &ethernets[lp->endmax].panel;
	    if(lpp->displayed && lpp->stats_changed)
		flush_1_linkstat(LT_ETHERNET, lpp, lp->endmax, lp->endmax);
	}
	else {
	    lpp	= &lp->panelmin;
	    if(lpp->displayed && lpp->stats_changed)
		flush_1_linkstat(LT_POINT2POINT, lpp, lp->endmin, lp->endmax);

	    lpp	= &lp->panelmax;
	    if(lpp->displayed && lpp->stats_changed)
		flush_1_linkstat(LT_POINT2POINT, lpp, lp->endmax, lp->endmin);
	}
    }
}

void flush_allstats(ClientData again)
{
    extern void flush_loadstats(CnetInt64, CnetInt64);

    flush_mainstats();
    flush_loadstats(STATS[(int)STATS_MSGDELIVERED], STATS[(int)STATS_FRAMESTX]);
    flush_nodestats();
    flush_linkstats();
    if((int)again)
	Tcl_CreateTimerHandler(STATS_FREQ /* yes, in millisecs */,
			(Tcl_TimerProc *)flush_allstats, (ClientData)TRUE);
}

#endif	/* defined(USE_TCLTK) */


/* ----------------------------------------------------------------------- */

void init_stats_layer(int z)
{
    int	n;

    zflag			= z;

    for(n=0 ; n<N_STATS ; ++n)
	STATS[n]		= int64_ZERO;
    STATS[(int)STATS_MSGRATE]	= int64_ONE;

    frame_txbytes		= int64_ZERO;
    msg_rxbytes			= int64_ZERO;

#if	defined(USE_TCLTK)
    if(Wflag)
	Tcl_CreateTimerHandler(STATS_FREQ /* yes, in millisecs */,
			(Tcl_TimerProc *)flush_allstats, (ClientData)TRUE);
#endif
}

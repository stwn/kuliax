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

/*  This file presents the "standard" application layer which is used
    (by default) if an alternative application layer is not requested
    with the  cnet -A  option.

    Functions in this file are called indirectly through their interfaces
    in applicationlayer.c (functions here have std_ prepended to their name).
    Wherever possible, the interface functions perform all error
    checking of the arguments, for example ensuring that a given node
    is a NT_HOST (and not a NT_ROUTER).

    This file should be used as a guide to writing any new application
    layers, such as those paying more attention to message size distributions
    and source/destination pairings. In particular, the following functions
    are required:

	void	std_init_application_layer(int Sflag)
	int	std_application_bounds(int *minmsg, int *maxmsg)
	int	std_reboot_application_layer(CnetInt64 *poll_next)
	int	std_poll_application(CnetInt64 *poll_next)

	int	std_CNET_read_application(CnetAddr *dest, char *msg, int *len);
	int	std_CNET_write_application(char *msg, int *len);

	int	std_CNET_enable_application (CnetAddr destaddr);
	int	std_CNET_disable_application(CnetAddr destaddr);

    In general, functions return 0 on success and -1 (and set cnet_errno)
    on failure. std_poll_application() returns TRUE or FALSE.

    This "standard" application layer uses nrand48() to provide a repeatable
    stream of random numbers and, hence, a repeatable stream of messages.
 */

#define	CnetAddr_UNKNOWN		(0xFFFFFFFF)

typedef	struct {
    int		to;		/* nodenumber (not address) */
    int		from;		/* nodenumber (not address) */
    int		checksum;
    int		len;
    int		session;
    int		seqno;
    CnetInt64	timecreated;
    char	msg[1];
} MESSAGE;

#define	MSG_HEADER_SIZE		(6*sizeof(int) + sizeof(CnetInt64))
#define	MSG_SIZE(m)		(MSG_HEADER_SIZE + m->len)

typedef struct {
    CnetAddr	msgdest;
    int		msglength;
    MESSAGE	*MSG;
    int		session;
    int		*seqsent;
    int		*seqrecv;
    int		*enabled;
    int		Nenabled;
    CnetInt64	poll_next;
} APPLICATION;


static	APPLICATION	**APPL;
static	APPLICATION	*ap;
static	char		**HOPS;
static	char		chargen[128], *endchargen;
static	unsigned short	xsubi[3];

#define	APPL_RAND48_SEED	2495693

#if	!defined(MIN)
#define	MIN(a,b)	((a<b) ? a : b)
#endif


/* ------------------------ Application Layer ----------------------------- */


static void count_hops(int start, int from, int hops)
{
    LINK	*lp;
    int		l, otherend;

    for(l=1 ; l<=NP[from].nlinks ; ++l) {
	lp	= &LP[ NP[from].links[l] ];

	if(lp->linktype == LT_POINT2POINT) {
	    otherend = (lp->endmin == from) ? lp->endmax : lp->endmin;

	    if(HOPS[start][otherend] >= (char)hops) {
		HOPS[start][otherend] = HOPS[otherend][start] = (char)hops;
		count_hops(start, otherend, hops+1);
	    }
	}
	else if(lp->linktype == LT_ETHERNET) {
	    ETHERNET	*ep	= &ethernets[lp->endmax];
	    ETHERNIC	*enp;
	    int		n;

	    for(n=0, enp=ep->nics ; n<ep->nnics ; ++n, ++enp) {
		otherend = enp->whichnode;
		if(otherend != from)
		    HOPS[otherend][from] = HOPS[from][otherend] = 1;
	    }
	}
    }
}


int std_application_bounds(int *minmsg, int *maxmsg)
{
    *minmsg	= sizeof(MESSAGE);
    *maxmsg	= MAX_MESSAGE_SIZE;
    return(0);
}

int std_init_application_layer(int Sflag)
{
    int		n, to;

    APPL = (APPLICATION **)malloc((unsigned)_NNODES * sizeof(APPLICATION *));
    memset((char *)APPL, 0, _NNODES * sizeof(APPLICATION *));

    HOPS		= (char **)malloc((unsigned)_NNODES * sizeof(char *));
    for(n=0 ; n<_NNODES ; ++n) {

	HOPS[n]		= (char *)malloc((unsigned)_NNODES * sizeof(char));
	for(to=0 ; to<_NNODES ; ++to)
	    HOPS[n][to]	= (char)(_NNODES+1);		/* infinity */
	HOPS[n][n]	= (char)0;

	if(NP[n].nodetype != NT_HOST)
	    continue;
	ap		=
	APPL[n]		= (APPLICATION *)malloc(sizeof(APPLICATION));
	ap->MSG		= (MESSAGE *)NULL;
	ap->session	= (n*n);
	ap->seqsent	= (int *)malloc((unsigned)_NNODES * sizeof(int));
	ap->seqrecv	= (int *)malloc((unsigned)_NNODES * sizeof(int));
	ap->enabled	= (int *)malloc((unsigned)_NNODES * sizeof(int));
    }

/*  CALCULATE HOP COUNT MATRIX  */
    for(n=0 ; n<_NNODES ; ++n)
	count_hops(n,n,1);

    endchargen = chargen;
    for(n=040 ; n<=0176 ; ++n)
	*endchargen++ = (char)n;

    Sflag	+= APPL_RAND48_SEED;
    memcpy((char *)xsubi, (char *)&Sflag, sizeof(Sflag));

    return(0);
}

/* ---------------------------------------------------------------------- */


/*  All of the following functions are called by, or on behalf of, an
    individual node, implicitly indicated with THISNODE, NP[THISNODE], etc.
 */


static CnetInt64 calc_poll_next()
{
    extern CnetInt64	poisson_usecs(CnetInt64, unsigned short *);
    CnetInt64		tmp64;

    tmp64 = WHICH64(NP[THISNODE].nattr.messagerate, DEFAULTNODE.messagerate);

    tmp64	= poisson_usecs(tmp64, xsubi);
    int64_ADD(tmp64, tmp64, TIMENOW_in_USEC);
    return(tmp64);
}


int std_reboot_application_layer(CnetInt64 *poll_next)
{
    int		n;

    ap			= APPL[THISNODE];
    if(ap->MSG)
	free((char *)ap->MSG);
    ap->MSG		= (MESSAGE *)NULL;
    ap->msgdest		= CnetAddr_UNKNOWN;
    ap->msglength	= UNKNOWN;
    ap->session++ ;
    ap->Nenabled	= 0;

    for(n=0 ; n<_NNODES ; ++n) {
	ap->seqsent[n]	=
	ap->seqrecv[n]	= 0;
	ap->enabled[n]	= FALSE;

	if(NP[n].nodetype == NT_HOST)
	    APPL[n]->seqsent[THISNODE]	=
	    APPL[n]->seqrecv[THISNODE]	= 0;
    }
    *poll_next = ap->poll_next = calc_poll_next();

    return(0);
}

int std_poll_application(CnetInt64 *poll_next)
{
    NODE	*np;
    int		hops, len, minsize, maxsize;

    ap		= APPL[THISNODE];

    if(int64_CMP(TIMENOW_in_USEC, <, ap->poll_next)) {	/* time for new msg? */
	*poll_next = ap->poll_next;
	return(FALSE);
    }

    *poll_next	= ap->poll_next = calc_poll_next();

    if(ap->Nenabled == 0)			/* not really want message? */
	return(FALSE);

    if(ap->MSG != (MESSAGE *)NULL)		/* already one ready? */
	return(TRUE);

/*  CHOOSE A RANDOM DESTINATION HOST (not a ROUTER) */

    if(ap->Nenabled == 1) {
	ap->msgdest = 0;
	while(ap->enabled[ap->msgdest] == FALSE)
	    ++ap->msgdest;		/* only enabled hosts (not a router) */
    }
    else {

/*
    HERE WE CHOOSE A DESTINATION HOST (not a router) THAT IS CLOSE.
    FIRSTLY, A HOST IS RANDOMLY CHOSEN; IF A DIRECT NEIGHBOUR IT REMAINS
    THE FINAL CHOICE. OTHERWISE, A MORE DISTANT NODE IS CHOSEN WITH A
    DECLINING PROBABILITY.
 */
	ap->msgdest = ((int)nrand48(xsubi)%_NNODES);
	for(;;) {
	    if(++ap->msgdest == (unsigned int)_NNODES) ap->msgdest = 0;
	    hops	= HOPS[THISNODE][ap->msgdest];
	    if(ap->enabled[ap->msgdest] == FALSE || hops > 30)
		continue;
	    if(hops == 1 || ((int)nrand48(xsubi) % (1<<hops)) == 0)
		break;
	}
    }

/*  FILL MESSAGE WITH SOME FICTICIOUS DATA */

    np		= &(NP[THISNODE]);

    minsize	= WHICH(np->nattr.minmessagesize, DEFAULTNODE.minmessagesize);
    maxsize	= WHICH(np->nattr.maxmessagesize, DEFAULTNODE.maxmessagesize);
    len			= (minsize - MSG_HEADER_SIZE) +
		  	  ((int)nrand48(xsubi) % (maxsize - minsize + 1));

    ap->MSG		= (MESSAGE *)malloc((unsigned)(MSG_HEADER_SIZE + len));
    ap->MSG->len	= len;

/*  THIS IS A SIMPLE CHARACTER GENERATOR WHICH BORROWS HEAVILY FROM
	~src/bsd/etc/inetd.c:chargen_stream()
 */
    {
	static char	*cs	= chargen;
	char		*mp	= ap->MSG->msg,
			*cp;
	int		i;

	if(cs++ >= endchargen)
	    cs	= chargen;
	cp	= cs;
	while(len > 0) {
	    i	= MIN(len, endchargen-cp);
	    memcpy(mp, cp, (unsigned)i);
	    mp	+= i;
	    len -= i;
	    if((cp += i) >= endchargen)
		cp	= chargen;
	}
    }

    ap->MSG->to			= ap->msgdest;
    ap->MSG->from		= THISNODE;
    ap->MSG->session		= ap->session;
    ap->MSG->seqno		= ap->seqsent[ap->msgdest]++;
    ap->MSG->timecreated	= TIMENOW_in_USEC;
    ap->msglength		= MSG_SIZE(ap->MSG);
    ap->MSG->checksum		= 0;
    ap->MSG->checksum		= checksum_ccitt((unsigned char *)ap->MSG,
							ap->msglength);
#if	defined(USE_TCLTK)
    inc_nodestats(0, ap->msglength);
#endif

    return(TRUE);
}


int std_CNET_read_application(CnetAddr *destaddr, char *msg, int *len)
{
    ap	= APPL[THISNODE];
    if(ap->MSG == (MESSAGE *)NULL) {		/* nothing pending! */
	cnet_errno	= ER_NOTREADY;
	goto bad_read;
    }

    if(*len < ap->msglength) {
	*len		= ap->msglength;
	cnet_errno	= ER_BADSIZE;
	goto bad_read;
    }

    *destaddr	= NP[ap->msgdest].nattr.address;	/* not nodenumber! */
    memcpy(msg,(char *)ap->MSG,(unsigned)ap->msglength);
    *len	= ap->msglength;

    inc_mainstats(STATS_MSGCREATED,0,0);

    ap->msgdest		= CnetAddr_UNKNOWN;		/* only give it once! */
    ap->msglength	= UNKNOWN;
    free((char *)ap->MSG);
    ap->MSG		= (MESSAGE *)NULL;

    return 0;

bad_read:
    *destaddr		= CnetAddr_UNKNOWN;
    *msg		= '\0';
    return (-1);
}


int std_CNET_write_application(char *msg, int *len)
{
    CnetInt64	tmp64;
    MESSAGE	header, *r;
    int		aligned, got_chk, true_chk, t;

    ap	= APPL[THISNODE];

/*  DETERMINE IF THE PROVIDED MESSAGE IS ON A LONG-INT BOUNDARY */
    aligned = (((long)msg % sizeof(long)) == 0);

    if(aligned)
	r	= (MESSAGE *)msg;
    else {							/* align it */
	memcpy(&header, msg, (unsigned)MSG_HEADER_SIZE);
	r	= &header;
    }

    if(*len < MSG_HEADER_SIZE || *len > MAX_MESSAGE_SIZE) {
	cnet_errno = ER_BADARG;
	goto bad_write;
    }
    if(r->len <= 0 || r->len > MAX_MESSAGE_SIZE) {
	cnet_errno = ER_CORRUPTDATA;
	goto bad_write;
    }

/*  NEXT WE CALCULATE THE CHECKSUM - CAREFUL IF MSG IS NOT ON AN INT BOUNDARY */
    if(aligned) {
	got_chk		= r->checksum;
	r->checksum	= 0;
    }
    else {
	memcpy(&got_chk,&(((MESSAGE *)msg)->checksum), sizeof(int));
	memset(&(((MESSAGE *)msg)->checksum), 0, sizeof(int));
    }
#if	_STDC_
    true_chk	= checksum_ccitt((unsigned char *)msg,(signed)MSG_SIZE(r));
#else
    true_chk	= checksum_ccitt((unsigned char *)msg,(int)MSG_SIZE(r));
#endif

/*  NOW, PUT THE CHECKSUM BACK IN ITS CORRECT POSITION */
    if(aligned)
	r->checksum	= got_chk;
    else
	memcpy(&(((MESSAGE *)msg)->checksum), &got_chk, sizeof(int));

    if(got_chk != true_chk) {
	cnet_errno	= ER_CORRUPTDATA;
	goto bad_write;
    }
    if(*len != (int)MSG_SIZE(r)) {
	cnet_errno = ER_BADSIZE;
	goto bad_write;
    }
    if(r->to != THISNODE) {
	cnet_errno = ER_NOTFORME;
	goto bad_write;
    }
    if(r->from < 0 || r->from >= _NNODES || r->from == THISNODE) {
	cnet_errno = ER_BADSENDER;
	goto bad_write;
    }

/*  Notice that I'm not treating the 'wrong session' error as an error
    which affects the statistics. Now, even with nodes crashing and rebooting,
    it should still be possible to keep that 100% delivery performance.
    (Students should implement their own Session Layer to handle their
     own session numbers, which are negotiated when a host reboots).
 */
    if(r->session != APPL[r->from]->session) {
	r->checksum	= got_chk;
	cnet_errno	= ER_BADSESSION;
	return(-1);
    }

    if(r->seqno != ap->seqrecv[r->from]) {
	cnet_errno = ER_OUTOFSEQ;
	goto bad_write;
    }
    ap->seqrecv[r->from]++;

#if	defined(USE_TCLTK)
    inc_nodestats(1, *len);
#endif
    int64_SUB(tmp64, TIMENOW_in_USEC, r->timecreated);
    int64_L2I(t, tmp64);
    inc_mainstats(STATS_MSGDELIVERED, t, r->len);
    return 0;

bad_write:
#if	defined(USE_TCLTK)
    inc_nodestats(2, abs(*len));
#endif
    inc_mainstats(STATS_MSGERROR,0,0);
    return (-1);
}


/* --------------    Enable/Disable Application Layer ------------------- */


int std_CNET_enable_application(CnetAddr destaddr)
{
    int	destnode;

    ap	= APPL[THISNODE];

    if(destaddr == ALLNODES) {
	ap->Nenabled		= 0;
	for(destnode=0 ; destnode<_NNODES ; ++destnode)
	    if(destnode != THISNODE && NP[destnode].nodetype == NT_HOST) {
		ap->enabled[destnode]	= TRUE;
		ap->Nenabled++;
	    }
	    else
		ap->enabled[destnode]	= FALSE;
    }
    else {
	for(destnode=0 ; destnode<_NNODES ; ++destnode)
	    if(NP[destnode].nattr.address == destaddr) {
		if(destnode != THISNODE && NP[destnode].nodetype == NT_HOST) {
		    if(ap->enabled[destnode] == FALSE)
			ap->Nenabled++;
		    ap->enabled[destnode] = TRUE;
		}
		break;
	    }
    }
    return(0);
}

int std_CNET_disable_application(CnetAddr destaddr)
{
    int	destnode;

    ap	= APPL[THISNODE];

    if(destaddr == ALLNODES) {
	for(destnode=0 ; destnode<_NNODES ; ++destnode)
	    ap->enabled[destnode]	= FALSE;
	ap->Nenabled		= 0;
    }
    else {
	for(destnode=0 ; destnode<_NNODES ; ++destnode)
	    if(NP[destnode].nattr.address == destaddr) {
		if(destnode >= 0 && destnode < _NNODES) {
		    if(destnode != THISNODE && ap->enabled[destnode] == TRUE)
			    ap->Nenabled--;
		    ap->enabled[destnode] = FALSE;
		}
		break;
	    }
    }
    return(0);
}

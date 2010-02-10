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


/*  This file presents the "standard" physical layer which is used
    (by default) if an alternative physical layer is not requested
    with the  cnet -P  option.

    Functions in this file are called indirectly through their interfaces
    in physicallayer.c (functions here have std_ prepended to their name).
    Wherever possible, the interface functions perform all error
    checking of the arguments, for example ensuring that a given frame
    has a length >= 0, etc.

    This file should be used as a guide to writing any new physical
    layers, such as those paying more attention to corruption and loss
    distributions.  In particular, the following functions are required:

	void	std_init_physical_layer(int eflag, int Sflag);
	int	std_reboot_physical_layer(int node);
	int	std_clear_physical_link(int linkno);
	int	std_poll_physical(CnetInt64 *when, int *node);

	int	std_CNET_read_physical(int *link, char *msg, int *len);
	int	std_CNET_write_physical(int link, char *msg, int *len);
	int	std_CNET_write_physical_reliable(int link, char *msg, int *len);
	int	std_CNET_write_direct(CnetAddr destaddr, char *msg, int *len);

    In general, functions return 0 on success and -1 (and set cnet_errno)
    on failure.

    This "standard" physical layer uses nrand48() to provide a repeatable
    stream of random numbers and, hence, repeatable corruption and loss.
 */

typedef struct _fq {
    struct _fq		*next;
    int			destnode;	/* index into NP[] */
    int			destlink;	/* dest link at NP[destnode] */
    int			linkno;		/* index into LP[] */
    CnetInt64		arrives;	/* time in usec the frame arrives */
    int			len;		/* len of data iff not corrupted */
    char		*frame;
    int			corrupted;
    long		fqseq;		/* filled in by PL_enqueue() */
} FRAMEQUEUE;

static	FRAMEQUEUE	GFQ;		/* global frame queue */
static	long		polled_seq;
static	int		*nframes;

static	int		eflag;		/* 2 flags passed to init_physical() */
static	int		Nflag;

static	unsigned short	xsubi[3];

#define	PHYS_RAND48_SEED	93802533


/* ------------------------- Physical Layer ----------------------------- */


int std_init_physical_layer(int _eflag, int _Nflag, int Sflag)
{
    int maxlinks, n;

    GFQ.next	= (FRAMEQUEUE *)NULL;
    polled_seq	= UNKNOWN;

    nframes = (int *)malloc((unsigned)_NNODES * sizeof(int));
    memset((char *)nframes, 0, _NNODES * sizeof(int));

    for(maxlinks=0, n=0 ; n<_NNODES ; ++n)
	if(maxlinks < NP[n].nlinks)
	    maxlinks = NP[n].nlinks;

    linkinfo	= (CnetLinkinfo *)malloc((unsigned)(maxlinks+1) *
					  sizeof(CnetLinkinfo));
    memset((char *)linkinfo, 0, (maxlinks+1) * sizeof(CnetLinkinfo));

    eflag	 = _eflag;
    Nflag	 = _Nflag;

    Sflag	+= PHYS_RAND48_SEED;
    memcpy((char *)xsubi, (char *)&Sflag, sizeof(Sflag));

    return(0);
}

/*  INDICATED NODE IS BEING REBOOTED, THROW AWAY ALL OF ITS PENDING FRAMES */
int std_reboot_physical_layer(int node)
{
    FRAMEQUEUE	*fq	= &GFQ;
    FRAMEQUEUE	*tmp;

    while(fq->next) {
	if(fq->next->destnode == node) {
	    tmp		= fq->next;
	    fq->next	= fq->next->next;
	    if(tmp->frame)
		free(tmp->frame);
	    free((char *)tmp);
	}
	else
	    fq	= fq->next;
    }
    nframes[node]	= 0;
    return(0);
}

/*  THIS LINK HAS JUST CRASHED, THROW AWAY ALL OF ITS PENDING FRAMES */
int std_clear_physical_link(int linkno)
{
    FRAMEQUEUE	*fq	= &GFQ;
    FRAMEQUEUE	*tmp;

    while(fq->next) {
	if(fq->next->linkno == linkno) {
	    tmp		= fq->next;
	    fq->next	= fq->next->next;
	    --nframes[tmp->destnode];
	    if(tmp->frame)
		free(tmp->frame);
	    free((char *)tmp);
	}
	else
	    fq	= fq->next;
    }
    return(0);
}


/* ---------------------------------------------------------------------- */

/*  NOW QUEUE THE MESSAGE FOR THE DESTINATION, SORTED BY ARRIVAL TIME */

static void PL_enqueue(FRAMEQUEUE *newf)
{
    FRAMEQUEUE	 *fq, *same;
    static long fqseq		= 0;

    newf->fqseq	= fqseq++;

    fq		= &GFQ;
    same	= (FRAMEQUEUE *)NULL;
    while(fq->next) {
	if(fq->next->destnode == newf->destnode &&
	   fq->next->destlink == newf->destlink)
	    same	= fq->next;
	fq	= fq->next;
    }

    if(same) {
	fq	= same;
	if(int64_CMP(newf->arrives, <, same->arrives)) {
	    int64_ADD(newf->arrives, same->arrives, int64_ONE);
	}
    }
    else
	fq	= &GFQ;

    while(fq->next) {
	if(int64_CMP(newf->arrives, <, fq->next->arrives)) {
	    newf->next	= fq->next;
	    break;
	}
	fq	= fq->next;
    }
    fq->next	= newf;
    ++nframes[newf->destnode];
}


int std_poll_physical(CnetInt64 *when, int *node)
{
    extern int	poll_ethernets(CnetInt64 *when,int *d,int *link,char *f,int *l);

/*  WE REMOVE THE LEADING FRAME IF IT WAS NOT READ LAST TIME  */
    if(GFQ.next && polled_seq == GFQ.next->fqseq) {
	FRAMEQUEUE	*tmp = GFQ.next;

	GFQ.next	= GFQ.next->next;
	--nframes[tmp->destnode];
	if(tmp->frame)
	    free(tmp->frame);
	free((char *)tmp);
    }

/*  POLL/RUN EACH ETHERNET UP TO NOW OR UNTIL IT HAS PACKETS TO DELIVER */
    if(nethernets > 0) {

	FRAMEQUEUE	*newf;
	CnetInt64	ewhen = int64_ZERO;
	char		epacket[ETH_MAXPACKET];
	int		edest, elink, elen;

	while(poll_ethernets(&ewhen, &edest, &elink, epacket, &elen)) {

/*  ADD ANY ETHERNET PACKETS TO THE PHYSICAL LAYER QUEUE */
	    newf		= NEW(FRAMEQUEUE);
	    newf->destnode	= edest;
	    newf->destlink	= elink;
	    newf->linkno	= AN_ETHERNET;
	    newf->arrives	= ewhen;
	    newf->len		= elen;
	    newf->frame		= (char *)malloc((unsigned)elen);
	    memcpy(newf->frame, epacket, (unsigned)elen);
	    newf->corrupted	= FALSE;
	    newf->next		= (FRAMEQUEUE *)NULL;
	    PL_enqueue(newf);
	    *when		= ewhen;
	}
    }

/*  FINALLY, REPLY TRUE/FALSE, INDICATING WHEN WHICH NODE IS ABLE TO RECEIVE */
    if(GFQ.next && int64_CMP(GFQ.next->arrives, <=, TIMENOW_in_USEC)) {
	*node		= GFQ.next->destnode;
	polled_seq	= GFQ.next->fqseq;
	return(TRUE);
    }
    else {
	if(GFQ.next && int64_CMP(GFQ.next->arrives, <, *when))
	    *when	= GFQ.next->arrives;
	polled_seq	= UNKNOWN;
	return(FALSE);
    }
}


/* --------------------- Unreliable Physical Layer ------------------------- */


int std_CNET_read_physical(int *link, char *frame, int *len)
{
    FRAMEQUEUE	*fq	= GFQ.next;

/*  ENSURE THAT THIS NODE WAS ACTUALLY CALLED VIA AN EVENT, AND THAT THEY
    ARE NOT JUST POLLING THE PHYSICAL LAYER */

    if(fq == (FRAMEQUEUE *)NULL	|| fq->fqseq != polled_seq) {
	*len		= 0;
	cnet_errno	= ER_NOTREADY;
    }
    else if(eflag && fq->corrupted) {
	GFQ.next	= GFQ.next->next;
	--nframes[fq->destnode];
	polled_seq	= UNKNOWN;

	*link		= fq->destlink;
	free((char *)fq);
	*frame		= '\0';
	*len		= 0;

	cnet_errno	= ER_CORRUPTDATA;
	return(-1);
    }

    else if(*len < fq->len) {
	*len		= fq->len;
	cnet_errno	= ER_BADSIZE;
    }
    else {
	*link		= fq->destlink;
	*len		= fq->len;
	memcpy(frame,fq->frame,(unsigned)*len);

	if(fq->linkno != AN_ETHERNET) {
#if	defined(USE_TCLTK)
	    inc_linkstats(NP[THISNODE].links[*link], 1, *len);	/*  received */
#endif
	    inc_mainstats(STATS_FRAMESRX, 0,0);
	}
	GFQ.next	= GFQ.next->next;
	polled_seq	= UNKNOWN;

	free(fq->frame);
	free((char *)fq);
	--nframes[THISNODE];

	return(0);
    }
    *link	= UNKNOWN;
    *frame	= '\0';

    return(-1);
}


/* ---------------------------------------------------------------------- */


static int hidden_write(int link, char *frame, int *len, int unreliable)
{
    extern int	write_ethernet(int link, char *frame, int len);

    FRAMEQUEUE	*newf;
    LINK	*thislink;
    LINKATTR	*thislinkattr;

    CnetInt64	Twrite, Tprop, delay, arrives, tmp64;
    int		destnode, destlink, linkno, tmp;
    int		is_lost		= FALSE;
    int		is_corrupt	= FALSE;

    if(*len <= 0) {
	cnet_errno	= ER_BADARG;
	return(-1);
    }

/*  ARE WE WRITING TO THE LOOPBACK LINK?  DELIVER IN 1 millisecond */
    if(link == 0) {
	newf			= NEW(FRAMEQUEUE);
	newf->destnode		= THISNODE;
	newf->destlink		= 0;
	newf->linkno		= UNKNOWN;
	int64_I2L(newf->arrives, 1000);
	int64_ADD(newf->arrives, TIMENOW_in_USEC, newf->arrives);
	newf->len		= *len;
	newf->frame		= (char *)malloc((unsigned)newf->len);
	memcpy(newf->frame,frame,(unsigned)newf->len);
	newf->corrupted		= FALSE;
	newf->next		= (FRAMEQUEUE *)NULL;

	PL_enqueue(newf);
	inc_mainstats(STATS_FRAMESTX,0,*len);
	return(0);
    }

/*  FIND TO WHICH NODE, and LINK (AND ITS ATTRIBUTES) WE ARE SENDING */
    linkno	= NP[THISNODE].links[link];
    thislink	= &LP[linkno];

    if(!thislink->linkup) {
	cnet_errno	= ER_LINKDOWN;
	return(-1);
    }

    if(thislink->linktype == LT_ETHERNET)
	return( write_ethernet(link, frame, *len) );

    if(thislink->endmin == THISNODE) {
	thislinkattr	= &(thislink->lattrmin);
	destnode	= thislink->endmax;
    }
    else {
	thislinkattr	= &(thislink->lattrmax);
	destnode	= thislink->endmin;
    }

    if(nframes[destnode] > MAX_PENDING_FRAMES) {
	cnet_errno	= ER_TOOBUSY;
	return(-1);
    }

    tmp = WHICH(thislinkattr->transmitbufsize, DEFAULTLINK.transmitbufsize);
    if(*len > tmp) {
	cnet_errno	= ER_BADSIZE;
	return(-1);
    }

    for(destlink=1 ; destlink <= NP[destnode].nlinks ; ++destlink)
	if(&(LP[ NP[destnode].links[destlink] ]) == thislink)
	    break;

/*  ARRIVAL_TIME = NOW + PROPAGATIONDELAY + (LENGTH_IN_BITS / BANDWIDTH_BPS) */
    Tprop	=
	WHICH64(thislinkattr->propagationdelay, DEFAULTLINK.propagationdelay);

    int64_I2L(Twrite, (*len * 8));
    int64_MUL(Twrite, Twrite, MILLION64);
    int64_I2L(tmp64, WHICH(thislinkattr->bandwidth, DEFAULTLINK.bandwidth));
    int64_DIV(Twrite, Twrite, tmp64);

    if(int64_CMP(thislinkattr->nextfree, <=, TIMENOW_in_USEC)) {
	delay	= int64_ZERO;
	int64_ADD(arrives, TIMENOW_in_USEC, Twrite);
	int64_ADD(arrives, arrives, Tprop);		/* sent right now */
	int64_ADD(thislinkattr->nextfree, TIMENOW_in_USEC, Twrite);
    }
    else {
	int64_SUB(delay, thislinkattr->nextfree, TIMENOW_in_USEC);
	int64_ADD(arrives, thislinkattr->nextfree, Twrite);
	int64_ADD(arrives, arrives, Tprop);		/* delayed send */
	int64_ADD(thislinkattr->nextfree, thislinkattr->nextfree, Twrite);
    }

#if	defined(USE_TCLTK)
    inc_linkstats(NP[THISNODE].links[link], 0, *len);	/*  transmitted */
#endif

/*  IF DESTINATION NODE IS NOT RUNNING, DON'T QUEUE OR "TRANSMIT" FRAME */
    if(NP[destnode].runstate != STATE_RUNNING)
	goto write_done;

/*  POSSIBLY LOSE FRAME (do not send it) */
    if(unreliable) {
	tmp = WHICH(thislinkattr->probframeloss, DEFAULTLINK.probframeloss);
	if(tmp > 0 && ((int)nrand48(xsubi) % (1<<tmp)) == 0) {
	    is_lost	= TRUE;
#if	defined(USE_TCLTK)
	    inc_linkstats(NP[THISNODE].links[link], 2, *len);	/*  errors */
#endif
	    inc_mainstats(STATS_FRAMESLOST,0,0);
	    goto write_done;
	}
    }

/*  PREPARE THE FRAME FOR ARRIVAL AT THE DESTINATION NODE */
    newf		= NEW(FRAMEQUEUE);
    newf->destnode	= destnode;
    newf->destlink	= destlink;
    newf->linkno	= linkno;
    newf->arrives	= arrives;
    newf->frame		= (char *)NULL;
    newf->len		= *len;
    newf->corrupted	= FALSE;
    newf->next		= (FRAMEQUEUE *)NULL;

/*  POSSIBLY CORRUPT FRAME (it will still be sent) */
    if(unreliable) {
	tmp		=
	    WHICH(thislinkattr->probframecorrupt, DEFAULTLINK.probframecorrupt);
	if(tmp > 0 && ((int)nrand48(xsubi) % (1<<tmp)) == 0) {

	    is_corrupt		= TRUE;
	    newf->corrupted	= TRUE;

#if	defined(USE_TCLTK)
	    inc_linkstats(NP[THISNODE].links[link], 2, *len);	/*  errors */
#endif
	    inc_mainstats(STATS_FRAMESCORRUPT,0,0);

/*  IF eflag IS SELECTED, WE WILL REPORT CORRUPTION BY RETURNING
    ER_CORRUPTDATA WHEN std_CNET_read_physical() IS CALLED.
    THUS, WHEN eflag IS SELECTED WE DON'T NEED TO PHYSICALLY CORRUPT THE FRAME
*/
	    if(!eflag) {
		int	i;
		char	*s;

		newf->frame	= (char *)malloc((unsigned)newf->len);
		memcpy(newf->frame,frame,(unsigned)newf->len);

#if	MAY_TRUNCATE_FRAMES
/*  CORRUPT FRAME BY REDUCING ITS LENGTH */
		i	= (int)nrand48(xsubi) % 8;
		if(i == 0)
		    newf->len = (int)(newf->len * 0.9);
		else
#endif
/*  CORRUPT FRAME BY CORRUPTING ITS CONTENTS */
		{
		    i		= (int)nrand48(xsubi) % (newf->len - 2);
		    s		= &newf->frame[i];
		    *s		= ~(*s);	/* found by all checksums() */
		    ++s;
		    *s		= ~(*s);
		}
	    }
	}
    }

    if(!newf->corrupted) {
	newf->frame		= (char *)malloc((unsigned)newf->len);
	memcpy(newf->frame,frame,(unsigned)newf->len);
    }

    PL_enqueue(newf);

write_done:

    tmp		=
	 WHICH(thislinkattr->costperframe, DEFAULTLINK.costperframe) +
	((*len) * WHICH(thislinkattr->costperbyte, DEFAULTLINK.costperbyte));
    inc_mainstats(STATS_FRAMESTX,tmp,*len);

#if	defined(USE_TCLTK)
    if(gattr.drawframes) {
	extern	void new_drawframe(int, char *, CnetInt64,CnetInt64, int,int);

	new_drawframe(*len, frame, delay, Tprop, is_lost, is_corrupt);
    }
#endif
    return 0;
}

/* ---------------------------------------------------------------------- */

int std_CNET_write_physical(int link, char *frame, int *len)
{
    return(hidden_write(link, frame, len, TRUE));	/* unreliable */
}


int std_CNET_write_physical_reliable(int link, char *frame, int *len)
{
    return(hidden_write(link, frame, len, FALSE));	/* !unreliable */
}


int std_CNET_write_direct(CnetAddr destaddr, char *frame, int *len)
{
    FRAMEQUEUE	*newf;
    int		destnode;

    if(*len <= 0) {
	cnet_errno	= ER_BADARG;
	return(-1);
    }
#if	defined(USE_TCLTK)
    inc_linkstats(NP[THISNODE].links[1], 0, *len);	/*  attribute link #1 */
#endif
    inc_mainstats(STATS_FRAMESTX,0,*len);

    for(destnode=0 ; destnode<_NNODES ; ++destnode)
	if(NP[destnode].nattr.address == destaddr)
	    break;

    if(destnode == _NNODES) {
	if(Nflag) {
	    cnet_errno	= ER_BADARG;
	    return(-1);
	}
	return 0;
    }
    if(NP[destnode].runstate != STATE_RUNNING)
	return 0;

    newf		= NEW(FRAMEQUEUE);
    newf->destnode	= destnode;
    newf->destlink	= 1;
    newf->linkno	= UNKNOWN;
    int64_ADD(newf->arrives, TIMENOW_in_USEC, int64_ONE);
    newf->len		= *len;
    newf->frame		= (char *)malloc((unsigned)newf->len);
    memcpy(newf->frame, frame, (unsigned)newf->len);
    newf->corrupted	= FALSE;
    newf->next		= (FRAMEQUEUE *)NULL;

    PL_enqueue(newf);
    return 0;
}

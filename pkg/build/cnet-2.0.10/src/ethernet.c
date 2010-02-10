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

#define	ETH_RAND48_SEED			93801089


ETHERNET		*ethernets	= (ETHERNET *)NULL;
int			nethernets	= 0;
static	int		npackets	= 0;	/* on all ethernets */

static	unsigned short	xsubi[3];
static	CnetInt64	ETHERTIME;

/*
static	CnetNicaddr	Z_ADDR	= { 0, 0, 0, 0, 0, 0 };
 */
static	CnetNicaddr	B_ADDR	= { 255, 255, 255, 255, 255, 255 };


/* --------------------------------------------------------------------- */


static void ETH_enqueue(ETHERNET *ep, ETHERNETQUEUE *new)
{
    ETHERNETQUEUE	 *eq	= &ep->EQ;

    while(eq->next) {
	if(int64_CMP(new->when, <, eq->next->when)) {
	    new->next	= eq->next;
	    break;
	}
	eq	= eq->next;
    }
    eq->next	= new;
}

static void collision_v2(ETHERNET *ep)
{
    ETHERNETQUEUE	*eq	= &ep->EQ;
    ETHERNETQUEUE	*delayed;
    ETHERNIC		*nic;
    CnetInt64		jamuntil;
    int			b;

    int64_I2L(jamuntil, ETH_JAMTIME);
    int64_ADD(jamuntil, jamuntil, ETHERTIME);

    while(eq->next) {
	if(int64_CMP(eq->next->when, >, jamuntil) && eq->next->status != TX) {
	    eq	= eq->next;
	    continue;
	}

	delayed			= eq->next;
	eq->next		= eq->next->next;
	nic			= &ep->nics[delayed->srcnic];

	switch (delayed->status) {
	case SENSING :
	    nic->status		= SENSING;
	    int64_I2L(delayed->when, ETH_SLOTTIME);
	    int64_ADD(delayed->when, delayed->when, jamuntil);
	    break;

	case BACKOFF :
	case ATTEMPT :
	case TX :
	    ++nic->ncollisions;
	    nic->status		= BACKOFF;
	    delayed->status	= BACKOFF;

/*  USE BINARY-EXPONENTIAL-BACKOFF TO EXTEND THE BACKOFF PERIOD */
	    b			= nic->backoff;
	    if(b < ETH_MAXBACKOFF)
		b		= ++nic->backoff;

	    int64_I2L(delayed->when, (nrand48(xsubi)%(1<<b)) * ETH_SLOTTIME);
	    int64_ADD(delayed->when, delayed->when, jamuntil);
	    ep->ncollisions++ ;
#if	defined(USE_TCLTK)
	    inc_linkstats(NP[nic->whichnode].links[delayed->srclink], 2, 1);
#endif
	    inc_mainstats(STATS_FRAMESCORRUPT, 0, 0);
	    break;
	default :
	    break;
	}
	delayed->next		= (ETHERNETQUEUE *)NULL;
	ETH_enqueue(ep, delayed);
    }
    ep->status		= IDLE;
    ep->nattempting	= 0;
}

static ETHERNETQUEUE *run_ethernet(ETHERNET *ep)
{
    ETHERNETQUEUE	*eq;
    ETHERNIC		*nic;
    ETHERNETQUEUE	*new;
    ETHERNETQUEUE	*deliver	= (ETHERNETQUEUE *)NULL;

    while(deliver == (ETHERNETQUEUE *)NULL) {
	eq	= &ep->EQ;
	if(eq->next == (ETHERNETQUEUE *)NULL)
	    break;

	ETHERTIME	= eq->next->when;
	if(int64_CMP(ETHERTIME, >, TIMENOW_in_USEC))
	    break;

	nic		= &ep->nics[eq->next->srcnic];
	switch (eq->next->status) {

	case SENSING :
	    new			= eq->next;
	    eq->next		= eq->next->next;
	    int64_I2L(new->when, ETH_SLOTTIME);
	    int64_ADD(new->when, new->when, ETHERTIME);
	    new->next		= (ETHERNETQUEUE *)NULL;

	    /* this node is sensing, move to another SENSE or ATTEMPT */
	    if(ep->status != TX) {
		new->status		= ATTEMPT;
		nic->status		= ATTEMPT;

		ep->status		= ATTEMPT;
		ep->nattempting++;
	    }
	    else
		;	/* someone else is transmitting, back to SENSING */

	    ETH_enqueue(ep, new);
	    break;

	case ATTEMPT :	/* this node at end of its tentative transmission */
	    if(ep->nattempting == 1) {		/* it's me, no collisions! */
		new			= eq->next;
		eq->next		= eq->next->next;

		new->status		= TX;
		nic->status		= TX;
		ep->status		= TX;

		int64_F2L(new->when, (new->len*8.0) / ETH_Mbps);
		int64_ADD(new->when, new->when, ETHERTIME);
		new->next		= (ETHERNETQUEUE *)NULL;

		ep->nattempting	= 0;
		ETH_enqueue(ep, new);
	    }
	    else	/* a collision, all ATTEMPT nodes now need to BACKOFF */
		collision_v2(ep);
	    break;

	case TX :
	    /* this node has successfully finished its full transmission */
	    nic->backoff	= 0;
	    ++nic->nsuccesses;
	    ++ep->nsuccesses;

	    nic->status	= IDLE;
	    ep->status	= IDLE;

	    deliver	= eq->next;
	    eq->next	= eq->next->next;

#if ETHERNICS_CAN_BUFFER
	    if(nic->head) {		/* next packet all ready to go */
		new		= nic->head;
		nic->head	= nic->head->next;
		if(nic->tail == new) {
		    nic->head	= (ETHERNETQUEUE *)NULL;
		    nic->tail	= (ETHERNETQUEUE *)NULL;
		}
		int64_I2L(new->when, ETH_SLOTTIME);
		int64_ADD(new->when, new->when, ETHERTIME);
		new->next	= (ETHERNETQUEUE *)NULL;

		new->status	= SENSING;
		nic->status	= SENSING;
		ETH_enqueue(ep, new);
	    }
#endif
	    break;

	case BACKOFF :	/* this node awoke from a BACKOFF, move to ATTEMPT */
	    if(ep->status == TX) {
		/* someone else is transmitting, a collision */
		nic->status		= ATTEMPT;
		eq->next->status	= ATTEMPT;
		ep->nattempting++;
		collision_v2(ep);
	    }
	    else {
		new			= eq->next;
		eq->next		= eq->next->next;

		new->status		= ATTEMPT;
		nic->status		= ATTEMPT;
		ep->status		= ATTEMPT;
		ep->nattempting++;

		int64_I2L(new->when, ETH_SLOTTIME);
		int64_ADD(new->when, new->when, ETHERTIME);
		new->next		= (ETHERNETQUEUE *)NULL;
		ETH_enqueue(ep, new);
	    }
	    break;

	default :
	    fprintf(stderr,"*** PANIC: bad eq->next->status=%d\n",
					eq->next->status);
	    exit(2);
	    break;
	}		/* switch (eq->next->status) */
    }
    return(deliver);
}


int poll_ethernets(CnetInt64 *when, int *dest,int *link,char *packet,int *len)
{
    static ETHERNETQUEUE	*deliver	= (ETHERNETQUEUE *)NULL;
    static int			whichether	= 0;
    static int			ethernic	= 0;

    ETHERNET		*ep;
    ETHERNIC		*nic;

    if(npackets == 0)
	return(FALSE);

/*  POLL EACH ETHERNET IN TURN  */
    while(npackets != 0 && whichether < nethernets) {
	ep	= &ethernets[whichether];
	if(deliver == (ETHERNETQUEUE *)NULL) {
	    deliver	= run_ethernet(ep);
	    if(deliver == (ETHERNETQUEUE *)NULL) {
		++whichether;
		ethernic = 0;
		continue;
	    }
	    ethernic	= 0;
	}
/*  A PACKET IS READY ON whichether, DELIVER IT TO INTERESTED NODES */
	while(ethernic < ep->nnics) {
	    LINKATTR	*lap;

	    nic		= &ep->nics[ethernic];
	    if(NP[nic->whichnode].runstate != STATE_RUNNING) {
		++ethernic;
		continue;
	    }
/*  DON'T DELIVER THE PACKET TO THE SENDING NODE */
	    if(ethernic == deliver->srcnic) {
		++ethernic;
		continue;
	    }

/*  DETERMINE WHICH OF THE DESTINATION'S LINKS THAT THIS PACKET ARRIVES AT */
	    lap		= &LP[nic->whichlink].lattrmin;

/*  DON'T DELIVER UNLESS PROMISCUOUS, A BROADCAST, OR FOR ME */
	    if(	lap->promiscuous == FALSE				&&
		memcmp(deliver->packet, B_ADDR, LEN_NICADDR) != 0	&&
		memcmp(deliver->packet, lap->nicaddr, LEN_NICADDR) != 0) {

		++ethernic;
		continue;
	    }
/*  PACKET IS DELIVERED AT A TIME RELATED TO DISTANCE BETWEEN src AND dest */
	    int64_I2L(*when,
		  (ETH_SLOTTIME/ep->nnics+1)*abs(deliver->srcnic-ethernic) -
		   ETH_SLOTTIME);
	    int64_ADD(*when, deliver->when, *when);

	    *dest	= nic->whichnode;
	    *link	= nic->nodeslink;
	    memcpy(packet, deliver->packet, deliver->len);
	    *len	= deliver->len;
	    ++ethernic;
	    return(TRUE);
	}
#if	defined(USE_TCLTK)
	nic	= &ep->nics[deliver->srcnic];
    inc_linkstats(NP[nic->whichnode].links[deliver->srclink],1,deliver->len);
#endif
	inc_mainstats(STATS_FRAMESRX, 0, deliver->len);
	free(deliver->packet);
	free(deliver);
	deliver		= (ETHERNETQUEUE *)NULL;
	--npackets;

	++whichether;
	ethernic = 0;
    }

/*  NO MORE PACKETS READY ON ANY ETHERNET. RESET FOR NEXT POLL */
    deliver	= (ETHERNETQUEUE *)NULL;
    whichether	= 0;
    ethernic	= 0;
    if(npackets) {
	int64_I2L(*when, 2*ETH_SLOTTIME);
	int64_ADD(*when, *when, TIMENOW_in_USEC);
    }
    return(FALSE);
}


/* --------------------------------------------------------------------- */


void extend_ethernet(int thisether, int whichnode, CnetNicaddr nicaddr)
{
    extern int		add_link(CnetLinktype, int, int, CnetNicaddr);

    ETHERNET		*ep;
    ETHERNIC		*nic;

    ep			= &ethernets[thisether];
    ep->nics		= realloc(ep->nics, (ep->nnics+1)*sizeof(ETHERNIC));
    nic			= &ep->nics[ep->nnics];
    memset(nic, 0, sizeof(ETHERNIC));
    nic->status		= IDLE;
    add_link(LT_ETHERNET, whichnode, thisether, nicaddr);

    nic->whichlink	= _NLINKS-1;
    nic->whichnode	= whichnode;
    nic->nodeslink	= NP[whichnode].nlinks;
    memcpy(nic->nicaddr, nicaddr, LEN_NICADDR);
    ++ep->nnics;
}

int new_ethernet(char *name)		/* called from parser.c */
{
    ETHERNET		*ep;

    if(nethernets == 0) {
	int	s	= ETH_RAND48_SEED;

	memset((char *)xsubi, 0, sizeof(xsubi));
	memcpy((char *)xsubi, (char *)&s, sizeof(s));
	ethernets	= NEW(ETHERNET);
    }
    else
	ethernets	=
	    (ETHERNET *)realloc(ethernets, (nethernets+1)*sizeof(ETHERNET));

    ep			= &ethernets[nethernets];
    memset(ep, 0, sizeof(ETHERNET));
    ep->name		= strdup(name);
    ep->status		= IDLE;
    ep->nics		= malloc(4*sizeof(ETHERNIC));
    return(nethernets++);
}


/* --------------------------------------------------------------------- */


int write_ethernet(int link, char *packet, int len)
{
    ETHERNET		*ep;
    ETHERNETQUEUE	*new;
    ETHERNIC		*nic;
    int			srcnic;
    int			linkno;

    if(len < ETH_MINDATA || len > ETH_MAXDATA) {
	cnet_errno	= ER_BADSIZE;
	return(-1);
    }

    linkno		= NP[THISNODE].links[link];
    ep			= &ethernets[LP[linkno].endmax];

    for(srcnic=0, nic=ep->nics ; srcnic<ep->nnics ; ++srcnic, ++nic)
	if(nic->whichnode == THISNODE)
	    break;
    if(srcnic == ep->nnics) {
	cnet_errno	= ER_OK;
	return(-1);
    }

    new			= NEW(ETHERNETQUEUE);
    new->srcnic		= srcnic;
#if	defined(USE_TCLTK)
    new->srclink	= link;
#endif

    new->packet		= (char *)malloc(len);
    memcpy(new->packet, packet, len);
    new->len		= len;
    new->next		= (ETHERNETQUEUE *)NULL;

    if(nic->status == IDLE) {		/* ready to begin sequence */
	int64_I2L(new->when, ETH_SLOTTIME);
	int64_ADD(new->when, new->when, TIMENOW_in_USEC);
	nic->backoff	= 0;

	if(ep->status == TX) {		/* some other NIC transmitting */
	    new->status	= SENSING;
	    nic->status	= SENSING;
	}
	else {				/* IDLE or ATTEMPT transmissions */
	    new->status	= ATTEMPT;
	    nic->status	= ATTEMPT;
	    ep->status	= ATTEMPT;
	    ep->nattempting++ ;
	}
	ETH_enqueue(ep, new);
    }

    else {				/* this NIC already transmitting */
#if ETHERNICS_CAN_BUFFER
	if(nic->head == (ETHERNETQUEUE *)NULL) { /* NIC's queue empty? */
	    nic->head		= new;
	    nic->tail		= new;
	}
	else {				/* no, just append to queue's tail */
	    nic->tail->next	= new;
	    nic->tail		= new;
	}
#else
	cnet_errno	= ER_TOOBUSY;
	return(-1);
#endif
    }

#if	defined(USE_TCLTK)
    inc_linkstats(NP[THISNODE].links[link], 0, len);  /* gone */
#endif
    inc_mainstats(STATS_FRAMESTX, 0, len);

    ++npackets;
    return(0);
}

/* --------------------------------------------------------------------- */


static int hex_to_int(int ch)
{
    if(ch >= '0' && ch <= '9')	return(ch-'0');
    if(ch >= 'a' && ch <= 'f')	return(10 + ch-'a');
    if(ch >= 'A' && ch <= 'F')	return(10 + ch-'A');
    return(0);
}

int CNET_parse_nicaddr(CnetNicaddr nicaddr, char *buf)
{
    int		c, n;

    for(n=0 ; n<LEN_NICADDR ; ++n) {
	if(!isxdigit((int)*buf))
	    break;
	c			= hex_to_int(*buf++);
	if(!isxdigit((int)*buf))
	    break;
	nicaddr[n]		= c*16 + hex_to_int(*buf++);

	if(*buf == ':' || *buf == '-')
	    ++buf;
    }
    if(n == LEN_NICADDR && *buf == '\0')
	return(0);
    memset(nicaddr, 0, LEN_NICADDR);
    return(1);
}

int CNET_format_nicaddr(char *buf, CnetNicaddr nicaddr)
{
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
		    nicaddr[0], nicaddr[1], nicaddr[2], 
		    nicaddr[3], nicaddr[4], nicaddr[5] );
    return(0);	/* what can go wrong? */
}


/* --------------------------------------------------------------------- */


int check_ethernets(BOOL **adj)
{
    ETHERNET	*ep1, *ep2;
    ETHERNIC	*nic1, *nic2;
    int		e1, e2, n1, n2;
    int		len, maxlen=0, x;

/*  WARN IF ANY ETHERNET ADDRESSES ARE NOT UNIQUE */
    for(e1=0, ep1=ethernets ; e1<(nethernets-1) ; ++e1, ++ep1)
	for(n1=0, nic1=ep1->nics ; n1<ep1->nnics ; ++n1, ++nic1)
	    for(e2=e1+1, ep2=ep1+1 ; e2<nethernets ; ++e2, ++ep2)
		for(n2=0, nic2=ep2->nics ; n2<ep2->nnics ; ++n2, ++nic2)
		    if(memcmp(nic1->nicaddr, nic2->nicaddr, LEN_NICADDR) == 0) {
			char	buf[32];

			CNET_format_nicaddr(buf, nic1->nicaddr);
			fprintf(stderr,
			  "duplicate ethernet addresses (%s): %s.%s and %s.%s\n",
			    buf,
			    ep1->name, NP[nic1->whichnode].nodename,
			    ep2->name, NP[nic2->whichnode].nodename );
			++nerrors;
		    }

/*  ESTABLISH ENTRIES IN THE WHOLE NETWORK'S ADJACENCY MATRIX */
    for(e1=0, ep1=ethernets ; e1<nethernets ; ++e1, ++ep1) {
	if(ep1->nnics < 2) {
	    fprintf(stderr, "Ethernet segment %s must have >= 2 NICs\n",
			ep1->name);
	    ++nerrors;
	}
	for(n1=0, nic1=ep1->nics ; n1<ep1->nnics ; ++n1, ++nic1)
	    for(n2=n1+1, nic2=&ep1->nics[n2] ; n2<ep1->nnics ; ++n2, ++nic2)

		adj[nic1->whichnode][nic2->whichnode] =
		adj[nic2->whichnode][nic1->whichnode] = TRUE;
    }

/*  GIVE EACH SEGMENT MEANINGFUL COORDINATES IF NOT ALREADY ASSIGNED */
    for(e1=0, ep1=ethernets ; e1<nethernets ; ++e1, ++ep1) {

	if(ep1->x == 0 || ep1->y == 0) {
	    for(n1=0, nic1=ep1->nics ; n1<ep1->nnics ; ++n1, ++nic1)

/*  IF ANY NIC ALREADY HAS COORDINATES, POSITION THIS SEGMENT WRT THAT NIC */
		if(NP[nic1->whichnode].nattr.x != 0	&&
		   NP[nic1->whichnode].nattr.y != 0) {

		    ep1->x	= NP[nic1->whichnode].nattr.x -
					(n1+1)*DEF_NODE_X + DEF_NODE_X/2;
		    ep1->y	= NP[nic1->whichnode].nattr.y +
					(((n1%2) == 0) ? 1 : -1) * DEF_NODE_Y;
		    break;
		}

/*  IF NO COORDINATES YET, POSITION SEGMENT RELATIVE TO OTHER SEGMENTS */
	    if(ep1->x == 0 || ep1->y == 0) {
		ep1->x	= (e1/2+1)*DEF_NODE_X + (e1/2)*maxlen;
		ep1->y	= 2*DEF_NODE_Y + (e1%2)*4*DEF_NODE_Y;
	    }
	}
	len		= ep1->nnics * DEF_NODE_X;
	if(maxlen < len)
	    maxlen	= len;
    }

/*  FINALLY, POSITION ALL OF THIS SEGMENT'S NICs THAT DO NOT HAVE COORDINATES */
    for(e1=0, ep1=ethernets ; e1<nethernets ; ++e1, ++ep1) {
	x	= ep1->x + DEF_NODE_X/2;
	for(n1=0, nic1=ep1->nics ; n1<ep1->nnics ; ++n1, ++nic1) {
	    if(NP[nic1->whichnode].nattr.x == 0)
		NP[nic1->whichnode].nattr.x	= x;
	    x	+= DEF_NODE_X;
	    if(NP[nic1->whichnode].nattr.y == 0)
		NP[nic1->whichnode].nattr.y	= (ep1->y-DEF_NODE_Y) +
						  (n1 % 2) * 2 * DEF_NODE_Y;
	}
    }
    return(0);
}


/* --------------------------------------------------------------------- */

void print_ethernets(FILE *fp)
{
    ETHERNET	*ep;
    ETHERNIC	*nic;
    int		e, n;

    for(e=0, ep=ethernets ; e<nethernets ; ++e, ++ep) {
	fprintf(fp, "ethernet %s {\n", ep->name);
	fprintf(fp, "    x=%d, y=%d\n\n", ep->x, ep->y);
	for(n=0, nic=ep->nics ; n<ep->nnics ; ++n, ++nic) {
	    char	buf[32];

	    CNET_format_nicaddr(buf, nic->nicaddr);
	    fprintf(fp, "    %s\t%s {}\n", buf, NP[nic->whichnode].nodename );
	}
	fprintf(fp, "}\n\n");
    }
}

#if	defined(USE_TCLTK)
static void draw1ethernet(ETHERNET *ep)
{
    ETHERNIC	*nic;
    int		n, x, y;

    x	= ep->x + ep->nnics*DEF_NODE_X;
/*  DRAW THE MAIN ETHERNET SEGMENT */
    TCLTK(".%s.map create line %d %d %d %d -fill %s -width %d -tags ls",
		argv0,
		ep->x, ep->y,
		x, ep->y,
		COLOUR_ETHERNET, CANVAS_FATLINK/2 );
/*  DRAW THE TWO TERMINATORS ON THE SEGMENT */
    TCLTK(".%s.map create line %d %d %d %d -fill %s -width %d -tags ls",
		argv0,
		ep->x, ep->y-CANVAS_FATLINK/2,
		ep->x, ep->y+CANVAS_FATLINK/2,
		COLOUR_ETHERNET, CANVAS_FATLINK/2 );
    TCLTK(".%s.map create line %d %d %d %d -fill %s -width %d -tags ls",
		argv0,
		x, ep->y-CANVAS_FATLINK/2,
		x, ep->y+CANVAS_FATLINK/2,
		COLOUR_ETHERNET, CANVAS_FATLINK/2 );
/*  AND THE NAME OF THE SEGMENT */
    TCLTK(
    ".%s.map create text %d %d -anchor w -font $stdiofont -tags ls -text %s",
		argv0,
		ep->x, ep->y+8, ep->name);

/*  DRAW EACH NODE'S CONNECTION TO THE SEGMENT */
    x	= ep->x + DEF_NODE_X/2;
    for(n=0, nic=ep->nics ; n<ep->nnics ; ++n, ++nic) {
	y	= NP[nic->whichnode].nattr.y;

	if(x == NP[nic->whichnode].nattr.x)
	   TCLTK(".%s.map create line %d %d %d %d -fill %s -width %d -tags ls",
		argv0,
		NP[nic->whichnode].nattr.x, y,
		x, ep->y,
		COLOUR_ETHERNET, CANVAS_FATLINK/2 );
	else {
	    int	midy = ep->y + (ep->y < y ? 1 : -1)*DEF_NODE_Y/4;

	   TCLTK(".%s.map create line %d %d %d %d -fill %s -width %d -tags ls",
		argv0,
		x, ep->y,
		x, midy,
		COLOUR_ETHERNET, CANVAS_FATLINK/2 );
	   TCLTK(".%s.map create line %d %d %d %d -fill %s -width %d -tags ls",
		argv0,
		x, midy,
		NP[nic->whichnode].nattr.x, y,
		COLOUR_ETHERNET, CANVAS_FATLINK/2 );
	}

/*  AND THE LINK-NUMBER OF THIS CONNECTION TO THE SEGMENT */
	TCLTK(
      ".%s.map create text %d %d -anchor c -font $stdiofont -tags n%d -text %d",
		argv0,
		x+8,
		ep->y-8 + (y<ep->y ? 0 : 1)*16,
		n, nic->nodeslink);
	x	+= DEF_NODE_X;
    }

    if(gattr.showcostperframe || gattr.showcostperbyte) {
#define	H2	6
#define	W2	3

	int	midx	= ep->x + (ep->nnics * DEF_NODE_X)/2;
	int	width	= W2 + 2;
	TCLTK(".%s.map create rectangle %d %d %d %d -fill %s -outline %s",
		    argv0, midx - width, ep->y - H2, midx + width, ep->y + H2,
		    "yellow", "black" );
	TCLTK(".%s.map create text %d %d -anchor c -font 5x7 -text \"%s\"",
		    argv0, midx, ep->y, "0");
#undef	W2
#undef	H2
    }
}

void draw_ethernets(void)
{
    ETHERNET	*ep;
    int		e;

    for(e=0, ep=ethernets ; e<nethernets ; ++e, ++ep)
	draw1ethernet(ep);
}
#endif

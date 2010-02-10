#include <cnet.h>
#include <stdlib.h>
#include "nltable.h"

/*  This file implements a better flooding algorithm which exhibits a bit
    more "intelligence" than the basic algorithm in flooding1.c .
    These additions, implemented with selective_flood(), include:

    1) data packets are initially transmitted on all links.
    2) packets are forwarded on all links except the one on which they arrived.
    3) acknowledgement packets are initially transmitted on the link on which
       their data packet arrived.

    selective_flood() accepts a bitmap indicating which links should be used.

    In addition, some simple routing statistics are maintained - the minimal
    round-trip number of hops and the average round-trip time in microseconds.

    This flooding algorithm exhibits a better efficiency than flooding1.c
    With the 7 nodes in the AUSTRALIA.MAP file, the efficiency is typically
    about 7%.

    Execute this protocol with the command:	cnet FLOODING2
 */


typedef enum    	{ NL_DATA, NL_ACK }   NL_PACKETKIND;

typedef struct {
    CnetAddr		src;
    CnetAddr		dest;
    NL_PACKETKIND	kind;      	/* only ever NL_DATA or NL_ACK */
    int			seqno;		/* 0, 1, 2, ... */
    int			hopsleft;	/* time to live */
    int			hopstaken;
    int			length;       	/* the length of the msg portion only */
    CnetInt64		timesent;	/* in microseconds */
    char		msg[MAX_MESSAGE_SIZE];
} NL_PACKET;

#define PACKET_HEADER_SIZE  (sizeof(NL_PACKET) - MAX_MESSAGE_SIZE)
#define PACKET_SIZE(p)	    (PACKET_HEADER_SIZE + p.length)


/* ----------------------------------------------------------------------- */


static int down_to_datalink(int link, char *packet, int length);

static void selective_flood(char *packet, int length, int links_wanted)
{
    int	   link;

    for(link=1 ; link<=nodeinfo.nlinks ; ++link)
	if( links_wanted & (1<<link) )
	    CHECK(down_to_datalink(link, packet, length));
}
			
static void down_to_network(CnetEvent ev, CnetTimer timer, CnetData data)
{
    NL_PACKET	p;

    p.length	= sizeof(p.msg);
    CHECK(CNET_read_application(&p.dest, p.msg, &p.length));
    CNET_disable_application(p.dest);

    p.src	= nodeinfo.address;
    p.kind	= NL_DATA;
    p.hopsleft	= MAXHOPS;
    p.hopstaken	= 1;
    p.timesent	= nodeinfo.time_in_usec;
    p.seqno	= NL_nextpackettosend(p.dest);

    selective_flood((char *)&p, PACKET_SIZE(p), ALL_LINKS);
}


static int up_to_network(char *packet, int length, int arrived_on)
{
    NL_PACKET	*p	= (NL_PACKET *)packet;

    if(p->dest == nodeinfo.address) {		/*  THIS PACKET IS FOR ME */
	if(p->kind == NL_DATA && p->seqno == NL_packetexpected(p->src)) {
	    CnetAddr	save;

	    length	= p->length;
	    CHECK(CNET_write_application(p->msg, &length));
	    NL_inc_packetexpected(p->src);

	    save	= p->src;	/* transform NL_DATA into NL_ACK */
	    p->src	= p->dest;
	    p->dest	= save;

	    p->kind	= NL_ACK;
	    p->hopsleft	= MAXHOPS;
	    p->length	= 0;
	    p->hopstaken++;
	    selective_flood(packet, PACKET_HEADER_SIZE, (1<<arrived_on) );
	}
	else if(p->kind == NL_ACK && p->seqno == NL_ackexpected(p->src)) {
	    CnetInt64	took;

	    CNET_enable_application(p->src);
	    NL_inc_ackexpected(p->src);
	    int64_SUB(took, nodeinfo.time_in_usec, p->timesent);
	    routing_stats(p->src, p->hopstaken, 0, took);
	}
    }
    else {				/* THIS PACKET IS FOR SOMEONE ELSE */
	if(--p->hopsleft > 0) {		/* send it back out again */
	    p->hopstaken++;
	    selective_flood(packet, length, ALL_LINKS & ~(1<<arrived_on) );
	}
    }
    return(0);
}

static void init_NL()
{
    init_NL_table(FALSE);
    CHECK(CNET_set_handler(EV_APPLICATIONREADY, down_to_network, 0));
}


/* ------------- A MINIMAL RELIABLE DATALINK LAYER FOLLOWS -------------- */


typedef struct {
    /* as we use a reliable datalink, we don't need any other fields */
    char        packet[sizeof(NL_PACKET)];
} DLL_FRAME;


static int down_to_datalink(int link, char *packet, int length)
{
    CHECK(CNET_write_physical_reliable(link, (char *)packet, &length));
    return(0);
}

static void up_to_datalink(CnetEvent ev, CnetTimer timer, CnetData data)
{
    DLL_FRAME	f;
    int		link, length;

    length	= sizeof(DLL_FRAME);
    CHECK(CNET_read_physical(&link, (char *)&f, &length));

    CHECK(up_to_network(f.packet, length, link));
}

static void init_DLL()
{
    CHECK(CNET_set_handler(EV_PHYSICALREADY,    up_to_datalink, 0));
}


/* ----------------------------------------------------------------------- */

void reboot_node(CnetEvent ev, CnetTimer timer, CnetData data)
{
    init_DLL();
    init_NL();
    CNET_enable_application(ALLNODES);
}

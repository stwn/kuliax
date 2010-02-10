#include <cnet.h>
#include <stdlib.h>
#include "nltable.h"

/*  This file implements a much better flooding algorithm than those in both
    flooding1.c and flooding2.c. As Network Layer packets are processed,
    the information in their headers is used to update the NL table.
    For example, each packet's hopsleft field is initially set to MAXHOPS but
    as the true distance between source and destination is learnt by the NL
    table, the hopsleft field is set to this much smaller value.  Similarly,
    as a new minimum hop count is determined by the NL table, the "best"
    link providing this minimum count is remembered and used thereafter.

    The extended routine routing_stats() is now called for both data and
    acknowledgement packets and we even "steal" information from Network
    Layer packets that don't belong to us!

    I don't think that it's a flooding algorithm any more Toto!

    This flooding algorithm exhibits an efficiency which improves over time
    (as the NL table "learns" more about shortest routes).  With the 7 nodes
    in the AUSTRALIA.MAP file, the initial efficiency is the same as that
    of flooding1.c (about 2.5%) but as the NL table information improves,
    the efficiency rises to over 65%.

    Execute this protocol with the command:	cnet FLOODING3
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
    p.hopsleft	= routing_maxhops(p.dest);
    p.hopstaken	= 1;
    p.timesent	= nodeinfo.time_in_usec;
    p.seqno	= NL_nextpackettosend(p.dest);

    selective_flood((char *)&p, PACKET_SIZE(p), routing_bestlink(p.dest));
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

	    routing_stats(p->src, p->hopstaken, arrived_on, 0);

	    save	 = p->src;	/* transform NL_DATA into NL_ACK */
	    p->src	 = p->dest;
	    p->dest	 = save;

	    p->kind	 = NL_ACK;
	    p->hopsleft	 = routing_maxhops(p->dest);
	    p->hopstaken = 1;
	    p->length	 = 0;
	    selective_flood(packet, PACKET_HEADER_SIZE,
				routing_bestlink(p->dest) );
	}
	else if(p->kind == NL_ACK && p->seqno == NL_ackexpected(p->src)) {
	    CnetInt64	took;

	    CNET_enable_application(p->src);
	    NL_inc_ackexpected(p->src);
	    int64_SUB(took, nodeinfo.time_in_usec, p->timesent);
	    routing_stats(p->src, p->hopstaken, arrived_on, took);
	}
    }
    else {				/* THIS PACKET IS FOR SOMEONE ELSE */
	if(--p->hopsleft > 0) {		/* send it back out again */
	    p->hopstaken++;
	    routing_stats(p->src, p->hopstaken, arrived_on, 0);
	    selective_flood(packet, length,
				routing_bestlink(p->dest) & ~(1<<arrived_on));
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

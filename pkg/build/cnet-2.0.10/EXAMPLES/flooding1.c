#include <cnet.h>
#include <stdlib.h>
#include "nltable.h"

/*  This is an implementation of the simplest (naive) flooding algorithm
    in cnet. Whenever a new network layer packet requires delivery, it is
    transmitted on *all* physical links (flooded).  To limit the combinatoric
    explosion in the number of data packets in the whole network, data
    packets are discarded after they have travelled more than MAXHOPS.

    The purpose of this example is to demonstrate the basic Network
    Layer flooding process itself, and for this reason only a minimal
    Data Link Layer protocol is provided to deliver the packets. As
    this flooding protocol assumes a reliable Data Link Layer protocol,
    CNET_write_physical_reliable() is called directly. Notice, however,
    the potential to separate the Network Layer and the Data Link Layer code.

    This basic flooding algorithm exhibits a very poor efficiency.
    With the 7 nodes in the AUSTRALIA.MAP file, the efficiency is typically
    only about 2.5%.

    Execute this protocol with the command:	cnet FLOODING1
 */


typedef enum    	{ NL_DATA, NL_ACK }   NL_PACKETKIND;

typedef struct {
    CnetAddr		src;
    CnetAddr		dest;
    NL_PACKETKIND	kind;      	/* only ever NL_DATA or NL_ACK */
    int			seqno;		/* 0, 1, 2, ... */
    int			hopsleft;	/* time to live */
    int			length;       	/* the length of the msg portion only */
    char		msg[MAX_MESSAGE_SIZE];
} NL_PACKET;

#define PACKET_HEADER_SIZE  (sizeof(NL_PACKET) - MAX_MESSAGE_SIZE)
#define PACKET_SIZE(p)	    (PACKET_HEADER_SIZE + p.length)


/* ----------------------------------------------------------------------- */


static int down_to_datalink(int link, char *packet, int length);

static void basic_flood(char *packet, int length)
{
    int	   link;

    for(link=1 ; link<=nodeinfo.nlinks ; ++link)
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
    p.seqno	= NL_nextpackettosend(p.dest);

    basic_flood((char *)&p, PACKET_SIZE(p));
}

static int up_to_network(char *packet, int length)
{
    NL_PACKET	*p;
    CnetAddr	addr;

    p	= (NL_PACKET *)packet;

    if(p->dest == nodeinfo.address) {		/*  THIS PACKET IS FOR ME */
	if(p->kind == NL_DATA && p->seqno == NL_packetexpected(p->src)) {
	    length	= p->length;
	    CHECK(CNET_write_application(p->msg, &length));
	    NL_inc_packetexpected(p->src);

	    addr	= p->src;	/* transform NL_DATA into NL_ACK */
	    p->src	= p->dest;
	    p->dest	= addr;

	    p->kind	= NL_ACK;
	    p->hopsleft	= MAXHOPS;
	    p->length	= 0;
	    basic_flood(packet, PACKET_HEADER_SIZE);	/* just an ACK */
	}
	else if(p->kind == NL_ACK && p->seqno == NL_ackexpected(p->src)) {
	    CNET_enable_application(p->src);
	    NL_inc_ackexpected(p->src);
	}
    }
    else {				/* THIS PACKET IF FOR SOMEONE ELSE */
	if(--p->hopsleft > 0)		/* send it back out again */
	    basic_flood(packet, length);
    }
    return(0);
}

static void init_NL()
{
    init_NL_table(TRUE);
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

    CHECK(up_to_network(f.packet, length));
}

static void init_DLL()
{
    CHECK(CNET_set_handler(EV_PHYSICALREADY, up_to_datalink, 0));
}


/* ----------------------------------------------------------------------- */

void reboot_node(CnetEvent ev, CnetTimer timer, CnetData data)
{
    init_DLL();
    init_NL();
    CNET_enable_application(ALLNODES);
}

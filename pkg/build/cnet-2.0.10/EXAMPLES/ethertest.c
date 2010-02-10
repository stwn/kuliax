#include <cnet.h>
#include <stdlib.h>
#include <string.h>

/*  This is a simple test of communication via a single Ethernet segment.
    To avoid the need for a physical->logical address discovery protocol,
    like ARP, we use CNET_set_nicaddr() to set NIC addresses to globally
    known values.  We then simply transmit to the required destination+link.
 */

typedef struct {
    CnetNicaddr		dest;
    CnetNicaddr		src;
    char		type[2];
    char		data[ETH_MAXDATA];
} ETHERPACKET;

#define	LEN_ETHERHEADER	(2*sizeof(CnetNicaddr) + 2)


static void application_ready(CnetEvent ev, CnetTimer timer, CnetData data)
{
    ETHERPACKET	packet;
    CnetAddr	destaddr;
    int         len;
    short int	twobytes;

    len		= sizeof(packet.data);
    CHECK(CNET_read_application(&destaddr, packet.data, &len));

/*  BUILD THE ADDRESS OF THE DESTINATION NIC (ONLY FOR THIS EXAMPLE) */
    memcpy(packet.dest, &destaddr, 4);
    packet.dest[4]	= 0;
    packet.dest[5]	= 1;
    memcpy(packet.src,  linkinfo[1].nicaddr, sizeof(CnetNicaddr));
    packet.type[0]	=
    packet.type[1]	= 0;

/*  TYPE CARRIES THE DATA'S TRUE LENGTH */
    twobytes = len;
    memcpy(packet.type, &twobytes, 2);

/*  POSSIBLY PAD SHORT PACKETS TO MINIMUM LENGTH */
    len		+= LEN_ETHERHEADER;
    if(len < ETH_MINPACKET)
	len	=  ETH_MINPACKET;
    CHECK(CNET_write_physical(1, (char *)&packet, &len));
}


static void physical_ready(CnetEvent ev, CnetTimer timer, CnetData data)
{
    ETHERPACKET	packet;
    int         link, len;
    short int	twobytes;

    len		= sizeof(packet);
    CHECK(CNET_read_physical(&link, (char *)&packet, &len));

/*  EXTRA THE DATA'S TRUE LENGTH (NOT THE LENGTH OF THE FULL PACKET */
    memcpy(&twobytes, packet.type, 2);
    len		= twobytes;
    CHECK(CNET_write_application(packet.data, &len));
}


void reboot_node(CnetEvent ev, CnetTimer timer, CnetData data)
{
    CnetNicaddr	nicaddr;
    int		l;

    for(l=1 ; l<=nodeinfo.nlinks ; ++l)
	if(linkinfo[l].linktype == LT_ETHERNET) {

/*  SET THE NIC ADDRESS OF EACH LINK TO A GLOBALLY KNOWN VALUE */
	    memcpy(nicaddr, &nodeinfo.address, 4);
	    nicaddr[4]	= 0;
	    nicaddr[5]	= l;
	    CHECK(CNET_set_nicaddr(l, nicaddr));
	}

    CHECK(CNET_set_handler(EV_APPLICATIONREADY, application_ready, 0));
    CHECK(CNET_set_handler(EV_PHYSICALREADY,    physical_ready, 0));

    CNET_enable_application(ALLNODES);
}

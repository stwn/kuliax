#include <cnet.h>

/*  This simple example demonstrates how a node can catch a change
    in a link's state with EV_LINKSTATE.

    If a link is changed with the right mouse button, or if linkmtbf
    and linkmttr are set in the topology file, link_changed() will be
    called with its third parameter set to the number of the link whose
    state has changed.

    This example determines if, and for how long, a node is isolated 
    from the rest of the network.
 */


static void link_changed(CnetEvent ev, CnetTimer t1, CnetData data)
{
    static int	ndown	= 0;
    static long	when;

    int link		= (int)data;

    if(linkinfo[ link ].linkup) {
	 if(ndown == nodeinfo.nlinks)
	    fprintf(stdout,"%s reconnected after %lds\n",
			nodeinfo.nodename, nodeinfo.time_of_day.sec - when);
	--ndown;
    }
    else {
	++ndown;
	if(ndown == nodeinfo.nlinks)
	    fprintf(stdout,"%s isolated!\n", nodeinfo.nodename);
	when	= nodeinfo.time_of_day.sec;
    }
}


void reboot_node(CnetEvent ev, CnetTimer t1, CnetData data)
{
    CHECK(CNET_set_handler(EV_LINKSTATE, link_changed, 0));
}

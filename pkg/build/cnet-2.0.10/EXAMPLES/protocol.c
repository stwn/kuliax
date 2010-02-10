#include <cnet.h>
#include <string.h>

/*  This function will be invoked each time the application layer has a
    message to deliver. Once we read the message from the application layer
    with CNET_read_application(), the application layer will be able to supply
    us with another message. The rate of new messages is controlled by
    the 'messagerate' node attribute.
 */

static void application_ready(CnetEvent ev, CnetTimer timer, CnetData data)
{
    CnetAddr   destaddr;
    int    length;
    char   buffer[MAX_MESSAGE_SIZE];

/*  Firstly, indicate the maximum sized message we are willing to receive */

    length = MAX_MESSAGE_SIZE;

/*  Then accept the message from the application layer.  We will be informed of
    the message's destination address and length and buffer will be filled in.
 */

    CNET_read_application(&destaddr, buffer, &length);

    printf("\tI have a message of %4d bytes for address %d\n",
			    length, (int)destaddr);
}


static void button_pressed(CnetEvent ev, CnetTimer timer, CnetData data)
{
    printf("\n Node number     : %d\n", nodeinfo.nodenumber);
    printf(" Node address    : %ld\n", nodeinfo.address);
    printf(" Node name       : %s\n", nodeinfo.nodename);
    printf(" Number of links : %d\n\n", nodeinfo.nlinks);
}

void reboot_node(CnetEvent ev, CnetTimer timer, CnetData data)
{

/*  Indicate that we are interested in hearing about the application layer
    having messages for delivery and indicate which function to invoke for them.
 */
    CNET_set_handler(EV_APPLICATIONREADY, application_ready, 0);
    CNET_enable_application(ALLNODES);

    CNET_set_handler(EV_DEBUG1, button_pressed, 0);
    CNET_set_debug_string(EV_DEBUG1, "Node Info");
}

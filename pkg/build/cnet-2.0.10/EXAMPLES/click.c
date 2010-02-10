#include <cnet.h>
#include <ctype.h>
#include <string.h>

/*  This is a simple cnet protocol source file which demonstrates the use
    of debugging buttons and the Physical Layer in cnet.

    The supplied function reboot_node() will first be called for each node.
    reboot_node() informs cnet that the node is interested in the EV_DEBUG1,
    EV_DEBUG2 and EV_PHYSICALREADY events.
    Strings are placed on the first and second debug buttons.

    When EV_DEBUG1 occurs, cnet will call send_reliable(), which will format a
    message in a frame for the other node and transmit it on link 1.
    As the message will be assumed to be a C string (ending in a NULL byte),
    it is important to send the NULL byte too (hence, strlen()+1 ).
    Similarly, when EV_DEBUG2 occurs, send_unreliable() is called.

    When EV_PHYSICALREADY occurs, cnet will call frame_arrived(), which
    will read the frame from the Physical Layer and print it out.

    Note the differing propagation delays in the topology file CLICK.
 */


#if     defined(SVR4) || defined(__svr4__)
extern long	lrand48(void);
#define		random()	lrand48()
#else
extern int	random(void);
#endif


static int  count    = 0;
static char *fruit[] = { "apple", "pineapple", "banana", "tomato", "plum" };

#define	NFRUITS		(sizeof(fruit) / sizeof(fruit[0]))


static void send_reliable(CnetEvent ev, CnetTimer timer, CnetData data)
{
    char	frame[256];
    int		length;

    sprintf(frame, "message %d is %s", ++count, fruit[random()%NFRUITS]);
    length	= strlen(frame) + 1;

    printf("sending %d bytes, checksum=%6d\n\n",
		    length, checksum_internet((unsigned short *)frame,length));
    CHECK(CNET_write_physical_reliable(1, frame, &length));
}


static void send_unreliable(CnetEvent ev, CnetTimer timer, CnetData data)
{
    char	frame[256];
    int		length;

    sprintf(frame, "message %d is %s", ++count, fruit[random()%NFRUITS]);
    length	= strlen(frame) + 1;

    printf("sending %d bytes, checksum=%6d\n\n",
		    length, checksum_internet((unsigned short *)frame,length));
    CHECK(CNET_write_physical(1, frame, &length));
}


static void frame_arrived(CnetEvent ev, CnetTimer timer, CnetData data)
{
    char	frame[256];
    int		link, length, i, ch;

    length	= sizeof(frame);
    CHECK(CNET_read_physical(&link, frame, &length));

    printf("    %d bytes arrived on link %d, checksum=%6d : \"",
	    length, link, checksum_internet((unsigned short *)frame,length));
    for(i=0 ; i<length-1 ; ++i) {
	ch = frame[i];
	if(!(isalnum(ch) || ch == ' '))
	    ch = '?';
	putchar(ch);
    }
    printf("\"\n\n");
}


void reboot_node(CnetEvent ev, CnetTimer timer, CnetData data)
{
/*  Indicate our interest in EV_DEBUG1, EV_DEBUG2 and EV_PHYSICALREADY events */

    CHECK(CNET_set_handler(EV_PHYSICALREADY, frame_arrived, 0));

    CHECK(CNET_set_handler(EV_DEBUG1,        send_reliable, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG1, "Reliable"));

    CHECK(CNET_set_handler(EV_DEBUG2,        send_unreliable, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG2, "Unreliable"));
}

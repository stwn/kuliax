#include <cnet.h>
#include <string.h>

/*  This is a simple cnet protocol source file which demonstrates the use
    of the keyboard event and the Physical Layer in cnet.

    To run this demo, run  cnet KEYBOARD  , click on both node icons to open
    their output windows, click the 'Run' button, click in one of the node's
    output windows, type your name and finally press RETURN.

    The supplied function reboot_node() will first be called for each node.
    reboot_node() informs cnet that the node is interested in the
    EV_KEYBOARDREADY and EV_PHYSICALREADY events.

    When EV_KEYBOARDREADY occurs, cnet will call keyboard(), which will
    read the input line using CNET_read_keyboard() and then transmit the line as
    a frame to the other node on link 1.
    As the frame will be assumed to be a C string (ending in a NULL byte),
    it is important to send the NULL byte too.

    When EV_PHYSICALREADY occurs, cnet will call frame_arrived(), which
    will read the frame from the Physical Layer and print it out.
    Note the differing propagation delays in the topology file KEYBOARD.

    Note that this simple ``protocol'' will not work if there is corruption
    on the link (probframecorrupt != 0). Can you see why?
 */


static void prompt(int inc)
{
    static int n=0;

    n += inc;
    printf("%s.%d> ", nodeinfo.nodename, n);
}

static void keyboard(CnetEvent ev, CnetTimer timer, CnetData data)
{
    char	line[80];
    int		length;

    length	= sizeof(line);
    CHECK(CNET_read_keyboard(line, &length));

    if(length > 1) {			/* not just a blank line? */
	printf("\tsending %d bytes - \"%s\"\n", length, line);
	CHECK(CNET_write_physical(1, line, &length));
	prompt(1);
    }
    else
	prompt(0);
}


static void frame_arrived(CnetEvent ev, CnetTimer timer, CnetData data)
{
    char frame[256];
    int  link, length;

    length	= sizeof(frame);
    CHECK(CNET_read_physical(&link, frame, &length));

    printf("\treceived %d bytes on link %d - \"%s\"\n",length,link,frame);
    prompt(0);
}


void reboot_node(CnetEvent ev, CnetTimer timer, CnetData data)
{
/*  Indicate our interest in certain cnet events */

    CHECK(CNET_set_handler( EV_KEYBOARDREADY, keyboard, 0));
    CHECK(CNET_set_handler( EV_PHYSICALREADY, frame_arrived, 0));

    prompt(1);
}

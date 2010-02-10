#include <cnet.h>

/*  This is a simple cnet protocol source file which demonstrates the use
    of timer events in cnet.

    The supplied function reboot_node() will first be called for each node.
    reboot_node() informs cnet that the node is interested in the EV_TIMER1
    events and that cnet should call timeouts() each time EV_TIMER1 occurs.
    Finally, reboot_node() requests that EV_TIMER1 occurs in 1000000us (1s).

    When EV_TIMER1 occurs, cnet will call timeouts(), which will print
    either tick or tock. As each timer event occurs only once, timeouts()
    must finally reschedule EV_TIMER1 to occur again in another 1000000us.
 */


void timeouts(CnetEvent ev, CnetTimer timer, CnetData data)
{
    static int which = 0;
    CnetInt64  when;

    printf("%3d.\t%s\n", which, (which%2) == 0 ? "tick" : "\ttock");
    ++which;

/*  Reschedule EV_TIMER1 to occur again in 1sec */

    int64_I2L(when, 1000000);
    CNET_start_timer(EV_TIMER1, when, 0);
}


void reboot_node(CnetEvent ev, CnetTimer timer, CnetData data)
{
    CnetInt64  when;

/*  Indicate that we are interested in the EV_TIMER1 event */

    CHECK(CNET_set_handler(EV_TIMER1, timeouts, 0));

/*  Request that EV_TIMER1 occur in 1sec, ignoring the return value */

    int64_I2L(when, 1000000);
    CNET_start_timer(EV_TIMER1, when, 0);
}

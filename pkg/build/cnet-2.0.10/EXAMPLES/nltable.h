#include <cnet.h>

/*  Packets that have travelled further than MAXHOPS are silently discarded.
    MAXHOPS should be increased for any topologies with a larger diameter.
 */

#define	MAXHOPS		4

/*  The Boolean bitmap ALL_LINKS, when stored as an integer, has all its bits
    set to '1', indicating that a packet should be transmitted on all links.
 */

#define	ALL_LINKS	(0xFFFFFFFF)

#if	!defined(FALSE)
#define	FALSE		0
#define	TRUE		1
#endif

extern	void	init_NL_table(int basic);

/*  FUNCTIONS MANAGING THE SEQUENCE NUMBERS OF PACKETS TO REMOTE DESTINATIONS */
extern	int	NL_nextpackettosend(CnetAddr destaddr);
extern	int	NL_ackexpected(CnetAddr destaddr);
extern	void	NL_inc_ackexpected(CnetAddr destaddr);
extern	int	NL_packetexpected(CnetAddr destaddr);
extern	void	NL_inc_packetexpected(CnetAddr destaddr);

/*  FUNCTIONS MANAGING THE ROUTING INFORMATION ABOUT REMOTE DESTINATIONS */
extern	int	routing_bestlink(CnetAddr destaddr);
extern	int	routing_maxhops(CnetAddr destaddr);
extern	void	routing_stats(CnetAddr addr,int hops,int link,CnetInt64 usec);

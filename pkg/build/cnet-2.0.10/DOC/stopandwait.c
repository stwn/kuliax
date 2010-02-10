#include <cnet.h>
#include <stdlib.h>
#include <string.h>

/*  This is an implementation of a stop-and-wait data link protocol.
    It is based on Tanenbaum's `protocol 4', 2nd edition, p227
    (or his 3rd edition, p205).
    This protocol employs only data and acknowledgement frames -
    piggybacking and negative acknowledgements are not used.

    It is currently written so that only one node (number 0) will
    generate and transmit messages and the other (number 1) will receive
    them. This restriction seems to best demonstrate the protocol to
    those unfamiliar with it.
    The restriction can easily be removed by "commenting out" the line

	    if(nodeinfo.nodenumber == 0)

    in reboot_node(). Both nodes will then transmit and receive (why?).

    Note that this file only provides a reliable data-link layer for a
    network of 2 nodes.
 */


typedef struct {
    char        data[MAX_MESSAGE_SIZE];
} MSG;

typedef enum    { DATA, ACK }   FRAMEKIND;

typedef struct {
    FRAMEKIND   kind;      	/* only ever DATA or ACK */
    int         len;       	/* the length of the msg field only */
    int         checksum;  	/* checksum of the whole frame */
    int         seq;       	/* only ever 0 or 1 */
    MSG         msg;
} FRAME;

#define FRAME_HEADER_SIZE  (sizeof(FRAMEKIND) + 3*sizeof(int))
#define FRAME_SIZE(f)      (FRAME_HEADER_SIZE + f.len)


static  MSG       	lastmsg;
static  int       	lastlength		= 0;
static  CnetTimer	lasttimer		= NULLTIMER;

static  int       	ackexpected		= 0;
static	int		nextframetosend		= 0;
static	int		frameexpected		= 0;


static void transmit_frame(MSG *msg, FRAMEKIND kind, int msglen, int seqno)
{
    FRAME       f;

    f.kind      = kind;
    f.seq       = seqno;
    f.checksum  = 0;
    f.len       = msglen;

    if(kind == ACK)
        printf("ACK transmitted, seq=%d\n",seqno);
    else if(kind == DATA) {
	CnetInt64	timeout;
	float		f1;

        memcpy(&f.msg, (char *)msg, msglen);
        printf(" DATA transmitted, seq=%d\n",seqno);

	int64_L2F(f1, linkinfo[1].propagationdelay);
        int64_F2L(timeout,
                3.0*(f1 + 1000000*(FRAME_SIZE(f) * 8.0)/linkinfo[1].bandwidth));

        lasttimer = CNET_start_timer(EV_TIMER1, timeout, 0);
    }
    msglen      = FRAME_SIZE(f);
    f.checksum  = checksum_ccitt((unsigned char *)&f, msglen);
    CHECK(CNET_write_physical(1, (char *)&f, &msglen));
}


static void application_ready(CnetEvent ev, CnetTimer timer, CnetData data)
{
    CnetAddr destaddr;

    lastlength  = sizeof(MSG);
    CHECK(CNET_read_application(&destaddr,(char *)&lastmsg,&lastlength));
    CNET_disable_application(ALLNODES);

    printf("down from application, seq=%d\n",nextframetosend);
    transmit_frame(&lastmsg, DATA, lastlength, nextframetosend);
    nextframetosend = 1-nextframetosend;
}


static void physical_ready(CnetEvent ev, CnetTimer timer, CnetData data)
{
    FRAME       f;
    int         link, len, checksum;

    len         = sizeof(FRAME);
    CHECK(CNET_read_physical(&link,(char *)&f,&len));

    checksum    = f.checksum;
    f.checksum  = 0;
    if(checksum_ccitt((unsigned char *)&f, len) != checksum) {
        printf("\t\t\t\tBAD checksum - frame ignored\n");
        return;           /* bad checksum, ignore frame */
    }

    if(f.kind == ACK) {
        if(f.seq == ackexpected) {
            printf("\t\t\t\tACK received, seq=%d\n",f.seq);
            CNET_stop_timer(lasttimer);
            ackexpected = 1-ackexpected;
            CNET_enable_application(ALLNODES);
        }
    }
    else if(f.kind == DATA) {
        printf("\t\t\t\tDATA received, seq=%d, ",f.seq);
        if(f.seq == frameexpected) {
            printf("up to application\n");
            len = f.len;
            CHECK(CNET_write_application((char *)&f.msg, &len));
            frameexpected = 1-frameexpected;
        }
        else
            printf("ignored\n");
        transmit_frame((MSG *)NULL, ACK, 0, f.seq);
    }
}


static void draw_frame(CnetEvent ev, CnetTimer timer, CnetData data)
{
    CnetDrawFrame *df	= (CnetDrawFrame *)data;
    FRAME	  *f	= (FRAME *)df->frame;

    if(f->kind == ACK) {
	df->colour[0]	= (f->seq == 0) ? CN_RED : CN_PURPLE;
	df->pixels[0]	= 10;
    }
    else if(f->kind == DATA) {
	df->colour[0]	= (f->seq == 0) ? CN_RED : CN_PURPLE;
	df->pixels[0]	= 10;
	df->colour[1]	= CN_GREEN;
	df->pixels[1]	= 30;
    }
}


static void timeouts(CnetEvent ev, CnetTimer timer, CnetData data)
{
    if(timer == lasttimer) {
        printf("timeout, seq=%d\n",ackexpected);
        transmit_frame(&lastmsg,DATA,lastlength,ackexpected);
    }
}


static void showstate(CnetEvent ev, CnetTimer timer, CnetData data)
{
    printf(
    "\n\tackexpected\t= %d\n\tnextframetosend\t= %d\n\tframeexpected\t= %d\n",
		    ackexpected, nextframetosend, frameexpected);

}


void reboot_node(CnetEvent ev, CnetTimer timer, CnetData data)
{
    if(nodeinfo.nodenumber > 1) {
	fprintf(stderr,"This is not a 2-node network!\n");
	exit(1);
    }

    CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));
    CHECK(CNET_set_handler( EV_PHYSICALREADY,    physical_ready, 0));
    CHECK(CNET_set_handler( EV_DRAWFRAME,        draw_frame, 0));
    CHECK(CNET_set_handler( EV_TIMER1,           timeouts, 0));
    CHECK(CNET_set_handler( EV_DEBUG1,           showstate, 0));

    CHECK(CNET_set_debug_string( EV_DEBUG1, "State"));

    if(nodeinfo.nodenumber == 1)
	CNET_enable_application(ALLNODES);
}

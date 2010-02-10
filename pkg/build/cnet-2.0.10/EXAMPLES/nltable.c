#include <cnet.h>
#include <stdlib.h>
#include "nltable.h"

/* ------ A SIMPLE NETWORK LAYER TABLE AS AN ABSTRACT DATA TYPE ------ */

typedef struct {
    CnetAddr	destaddr;

    int		ackexpected;
    int		nextpackettosend;
    int		packetexpected;

    int		minhops;
    int		minhop_link;
    CnetInt64	totaltime;
} NL_TABLE;

static	NL_TABLE	*table;
static	int		table_size;


static NL_TABLE *find_entry(CnetAddr destaddr)
{
    NL_TABLE	*tp;
    int		t;

    for(t=0, tp=table ; t<table_size ; ++t, ++tp)
	if(tp->destaddr == destaddr)
	    return(tp);

    table	= realloc((char *)table, (table_size+1)*sizeof(NL_TABLE));
    tp		= &table[table_size];

    tp->destaddr		= destaddr;
    tp->ackexpected		= 0;
    tp->nextpackettosend	= 0;
    tp->packetexpected		= 0;
    tp->minhops			= MAXHOPS;
    tp->minhop_link		= ALL_LINKS;
    tp->totaltime		= int64_ZERO;
    ++table_size;
    return(tp);
}

static void show_basic(CnetEvent ev, CnetTimer timer, CnetData data)
{
    NL_TABLE	*tp;
    int		t;

    CNET_clear();
    printf("\n%14s %14s %14s %14s\n",
	"destination","ackexpected","nextpackettosend", "packetexpected");
    for(t=0, tp=table ; t<table_size ; ++t, ++tp)
	printf("%14d %14d %14d %14d\n", (int)tp->destaddr,
		tp->ackexpected, tp->nextpackettosend, tp->packetexpected);
}

static void show_detailed(CnetEvent ev, CnetTimer timer, CnetData data)
{
    NL_TABLE	*tp;
    int		t;

    CNET_clear();
    printf("\n%14s %14s %14s %14s %14s\n",
    "destination","packets-ack'ed","min-hops","minhop-link", "round-trip-time");
    for(t=0, tp=table ; t<table_size ; ++t, ++tp)
	if(tp->ackexpected > 0) {
	    CnetInt64	avtime;

	    printf("%14d %14d %14d",
		(int)tp->destaddr, tp->ackexpected, tp->minhops);
	    if(tp->minhop_link == ALL_LINKS)
		printf(" %14s", "ALL-LINKS");
	    else
		printf(" %14d", tp->minhop_link);

	    int64_I2L(avtime, tp->ackexpected);
	    int64_DIV(avtime, tp->totaltime, avtime);
	    printf(" %14s\n", int64_L2A(avtime,0));
	}
}

/* ------------------------------------------------------------------- */

int NL_nextpackettosend(CnetAddr destaddr) {
    return(find_entry(destaddr)->nextpackettosend++);
}

int NL_ackexpected(CnetAddr destaddr) {
    return(find_entry(destaddr)->ackexpected);
}

void NL_inc_ackexpected(CnetAddr destaddr) {
    find_entry(destaddr)->ackexpected++;
}

int NL_packetexpected(CnetAddr destaddr) {
    return(find_entry(destaddr)->packetexpected);
}

void NL_inc_packetexpected(CnetAddr destaddr) {
    find_entry(destaddr)->packetexpected++;
}

/* ------------------------------------------------------------------- */

int routing_maxhops(CnetAddr destaddr) {
    return(find_entry(destaddr)->minhops);
}

int routing_bestlink(CnetAddr destaddr) {
    int link	= find_entry(destaddr)->minhop_link;
    return(link == ALL_LINKS ? ALL_LINKS : (1 << link));
}

void routing_stats(CnetAddr destaddr, int hops, int link, CnetInt64 usec)
{
    NL_TABLE	*tp;

    tp = find_entry(destaddr);
    if(tp->minhops >= hops) {
	tp->minhops	= hops;
	tp->minhop_link	= link;
    }
    int64_ADD(tp->totaltime, tp->totaltime, usec);
}
			 
void init_NL_table(int basic)
{
    if(nodeinfo.nlinks > 31) {
	fprintf(stderr,"Too many links for this routing table to support\n");
	exit(1);
    }

    table	= (NL_TABLE *)malloc(sizeof(NL_TABLE));
    table_size	= 0;

    if(basic) {
	CHECK(CNET_set_handler(EV_DEBUG1, show_basic, 0));
    }
    else {
	CHECK(CNET_set_handler(EV_DEBUG1, show_detailed, 0));
    }
    CHECK(CNET_set_debug_string( EV_DEBUG1, "NL info"));
}

#include "cnetheader.h"
#include <math.h>

/*  The cnet network simulator (v2.0.10)
    Copyright (C) 1992-2006, Chris McDonald

    Chris McDonald, chris@csse.uwa.edu.au
    Department of Computer Science & Software Engineering
    The University of Western Australia,
    Crawley, Western Australia, 6009
    PH: +61 8 9380 2533, FAX: +61 8 9380 1089.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define	P(n)		n, ((n)==1 ? "" : "s")

static	unsigned short	xsubi[3];

#define	TOP_RAND48_SEED	(6432867)

static	int		cflag;

void init_topology(int _cflag, char *Cflag, int Oflag,
		   int qflag,  char *Rflag, int Sflag, int tflag)
{
    NP				= NEW(NODE);
    LP				= NEW(LINK);
    _NNODES			= 0;
    _NLINKS			= 0;

    cflag			= _cflag;
    if(Cflag)
	DEFAULTNODE.compile	= Cflag;
    DEFAULTNODE.winopen		= Oflag;
    DEFAULTNODE.stdio_quiet	= qflag;
    if(Rflag)
	DEFAULTNODE.reboot_func	= Rflag;
    DEFAULTNODE.trace_all	= tflag;

    Sflag			+= TOP_RAND48_SEED;
    memcpy((char *)xsubi, (char *)&Sflag, sizeof(Sflag));
}


/* --------------------------------------------------------------------- */

static void init_nodeattrs(NODEATTR *na)
{
    na->minmessagesize	= DEFAULT;
    na->maxmessagesize	= DEFAULT;
    na->messagerate	= DEFAULT64;
    na->reboot_args	= DEFAULTNODE.reboot_args;
    na->reboot_func	= DEFAULTNODE.reboot_func;
    na->compile		= DEFAULTNODE.compile;
    na->osname		= DEFAULTNODE.osname;
    na->nodemtbf	= DEFAULT64;
    na->nodemttr	= DEFAULT64;
    na->winopen		= DEFAULTNODE.winopen;
}

static void init_linkattrs(LINKATTR *la)
{
    la->promiscuous	= FALSE;
    la->bandwidth	= DEFAULT;
    la->costperbyte	= DEFAULT;
    la->costperframe	= DEFAULT;
    la->probframecorrupt= DEFAULT;
    la->probframeloss	= DEFAULT;
    la->transmitbufsize	= DEFAULT;
    la->propagationdelay= DEFAULT64;
    la->linkmtbf	= DEFAULT64;
    la->linkmttr	= DEFAULT64;
    la->nextfree	= int64_ZERO;
}


int add_node(CnetNodetype nodetype, char *name, int defn, int *wasnew)
{
    extern int	strcasecmp(const char *, const char *);

    NODE	*np;
    int		 n;

    for(n=0 ; n<_NNODES ; n++)
	if(strcasecmp(name, NP[n].nodename) == 0) {
	    if(defn)
		NP[n].nodetype	= nodetype;	/* over-ride */
	    *wasnew		= FALSE;
	    return(n);				/* node seen before */
	}

    if(strlen(name) >= MAX_NODENAME_LEN) {
	fprintf(stderr,"%s: node name '%s' is too long (max is %d)\n",
				argv0, name, MAX_NODENAME_LEN-1);
	++nerrors;
	/* continue anyway */
    }

    NP = (NODE *)realloc((char *)NP, (unsigned)(_NNODES+1)*sizeof(NODE));
    np = &(NP[_NNODES]);

    memset((char *)np, 0, sizeof(NODE));
    np->nodetype		= nodetype;
    np->nodename		= strdup(name);
    np->nlinks			= 0;
    np->links			= NEW(int);
    np->runstate		= STATE_REBOOTING;

#if	(NODE_CLOCK_SKEW == 0)
    np->clock_skew	= int64_ZERO;
#else
    if(cflag)
	np->clock_skew	= int64_ZERO;
    else
	int64_I2L(np->clock_skew,
		  ((int)nrand48(xsubi)%(2*NODE_CLOCK_SKEW) - NODE_CLOCK_SKEW));
#endif

    init_nodeattrs(&(np->nattr));
    np->nattr.address		= (CnetAddr)(DEFAULTNODE.address + _NNODES);
    init_linkattrs(&(np->lattr));

    *wasnew			= TRUE;
    return( _NNODES++ );
}


void add_nodetype(CnetNodetype ntype, char *name, NODEATTR **na, LINKATTR **la)
{
    typedef struct _n {
	char		*name;
	CnetNodetype	 nodetype;
	NODEATTR	 na;
	LINKATTR	 la;
	struct _n	*next;
    } NTQ;

    static NTQ	*ntq	= (NTQ *)NULL;
    NTQ		*ntp	= ntq;

    while(ntp) {
	if(strcmp(name, ntp->name) == 0) {
	    fprintf(stderr,"%s: warning, nodetype '%s' redefined\n",
				argv0,name);
	    break;
	}
	ntp	= ntp->next;
    }
    ntp			= NEW(NTQ);
    ntp->name		= strdup(name);
    ntp->nodetype	= ntype;
    ntp->next		= ntq;
    ntq			= ntp;

    init_nodeattrs(&(ntq->na));
    init_linkattrs(&(ntq->la));
    *na			= &(ntq->na);
    *la			= &(ntq->la);
}


/* --------------------------------------------------------------------- */

int add_link(CnetLinktype linktype, int src, int dest, CnetNicaddr nicaddr)
{
    LINK	*lp;
    LINKATTR	*lap;
    int		t;

    if(linktype == LT_ETHERNET) {
	LP = (LINK *)realloc((char *)LP, (unsigned)(_NLINKS+1)*sizeof(LINK));
	lp			= &LP[_NLINKS];
	memset((char *)lp, 0, sizeof(LINK));	/* clears linkattrs too */

	lp->linktype		= LT_ETHERNET;
	lp->linkup		= TRUE;
	lp->endmin		= src;
	lp->endmax		= dest;		/* the ethernet segment # */

	lap			= &lp->lattrmin;
	memcpy(lap->nicaddr, nicaddr, LEN_NICADDR);
	lap->bandwidth		= ETH_Mbps * 1000000;
	lap->transmitbufsize	= ETH_MAXPACKET;
	int64_I2L(lap->propagationdelay, ETH_SLOTTIME);

	t			= ++NP[src].nlinks;
	NP[src].links =
		(int *)realloc((char *)NP[src].links,(t+1)*sizeof(int));
	NP[src].links[t]	= _NLINKS;
    }
    else if(linktype == LT_POINT2POINT) {
	int minnode, maxnode, l;

	if(src < dest)
	    minnode = src,  maxnode = dest;
	else
	    minnode = dest, maxnode = src;

/*  CHECK IF WE'VE SEEN A POINT-TO-POINT LINK BETWEEN THESE TWO NODES BEFORE */
	for(l=0 ; l<_NLINKS ; l++)
	    if(LP[l].endmin == minnode && LP[l].endmax == maxnode)
		    return(l);

	LP = (LINK *)realloc((char *)LP, (unsigned)(_NLINKS+1)*sizeof(LINK));
	lp 		= &LP[_NLINKS];
	memset((char *)lp, 0, sizeof(LINK));	/* clears linkattrs too */

	lp->linktype	= LT_POINT2POINT;
	lp->linkup	= TRUE;
	lp->endmin	= minnode;
	lp->endmax	= maxnode;
	lp->lattrmin	= NP[minnode].lattr;	/* copy node's default lattrs */
	lp->lattrmax	= NP[maxnode].lattr;

	t		= ++NP[minnode].nlinks;
	NP[minnode].links =
		(int *)realloc((char *)NP[minnode].links, (t+1)*sizeof(int));
	NP[minnode].links[t] = _NLINKS;

	t		= ++NP[maxnode].nlinks;
	NP[maxnode].links =
		(int *)realloc((char *)NP[maxnode].links, (t+1)*sizeof(int));
	NP[maxnode].links[t] = _NLINKS;

    }
    return( _NLINKS++ );
}

void add_linktype(CnetLinktype linktype, char *name, LINKATTR **la)
{
    typedef struct _l {
	char		*name;
	LINKATTR	 la;
	struct _l	*next;
    } LTQ;

    static LTQ	*ltq	= (LTQ *)NULL;
    LTQ		*ltp	= ltq;

    while(ltp) {
	if(strcmp(name, ltp->name) == 0) {
	    fprintf(stderr,"%s: warning, linktype '%s' redefined\n",
				argv0,name);
	    break;
	}
	ltp	= ltp->next;
    }
    ltp			= NEW(LTQ);
    ltp->name		= strdup(name);
    ltp->next		= ltq;
    ltq			= ltp;

    init_linkattrs(&(ltq->la));
    *la			= &(ltq->la);
}

/* -------------- generate a random (aesthetic?) topology -------------- */

#define	MAXDEGREE		4

static int connected(int nnodes, BOOL **adj)
{
    BOOL	**w;
    int		i, j, k;
    int		rtn	= TRUE;

    w	= (BOOL **)malloc(nnodes * sizeof(BOOL *));
    for(i=0 ; i<nnodes ; ++i) {
	w[i]	= (BOOL *)malloc(nnodes * sizeof(BOOL));
	for(j=0 ; j<nnodes ; ++j)
	    w[i][j] = adj[i][j];
    }

    for(k=0 ; k<nnodes ; ++k)
	for(i=0 ; i<nnodes ; ++i)
	    for(j=0 ; j<nnodes ; ++j)
		if(!w[i][j])
		    w[i][j] = w[i][k] && w[k][j];

    for(i=0 ; i<nnodes ; ++i)
	for(j=0 ; j<nnodes ; ++j)
	    if(!w[i][j]) {
		rtn	= FALSE;
		goto done;
	    }
done:
    for(i=0 ; i<nnodes ; ++i)
	free(w[i]);
    free(w);
    return(rtn);
}

#define	iabs(n)	((n)<0 ? -(n) : (n))

static void add1link(int n, int **grid, int nnodes, BOOL **adj)
{
    int		from, to;
    int		x, y;
    int		dx, dy;

    for(;;) {
	x	= (int)nrand48(xsubi)%n;
	y	= (int)nrand48(xsubi)%n;
	if(grid[x][y] == UNKNOWN)
	    continue;
	from	= grid[x][y];
	if(NP[from].nlinks == MAXDEGREE)
	    continue;

	do {
	    dx	= ((int)nrand48(xsubi)%3)-1;		/* dx:  -1, 0, +1 */
	    dy	= ((int)nrand48(xsubi)%3)-1;		/* dy:  -1, 0, +1 */
	}
#if	RANDOM_DIAGONALS
	while(dx == 0 && dy == 0);
#else
	while(iabs(dx) == iabs(dy));
#endif

	x	+= dx;
	y	+= dy;
	while(x>=0 && x<n && y>=0 && y<n) {
	    if(grid[x][y] != UNKNOWN) {
		to	= grid[x][y];
		if(NP[to].nlinks == MAXDEGREE || adj[from][to] == TRUE)
		    break;
		add_link(LT_POINT2POINT, from, to, NULL);
		adj[from][to] = adj[to][from] = TRUE;
		return;
	    }
	    x	+= dx;
	    y	+= dy;
	}
    }
}

void random_topology(int nnodes)
{
    int		i, j, n, minlinks, wasnew;
    int		x, y;
    char	*fmt;
    int		**grid;
    BOOL	**adj;

         if(nnodes <= 10 )	fmt = "host%d";
    else if(nnodes <= 100)	fmt = "host%02d";
    else			fmt = "host%03d";

    n		= (int)sqrt((double)(nnodes-1)) + 1;
    grid	= (int **)malloc(n * sizeof(int *));
    for(i=0 ; i<n ; ++i) {
	grid[i]	= (int *)malloc(n * sizeof(int));
	for(j=0 ; j<n ; ++j)
	    grid[i][j] = UNKNOWN;
    }

    adj		= (BOOL **)malloc(nnodes * sizeof(BOOL *));
    for(i=0 ; i<nnodes ; ++i) {
	adj[i]	= (BOOL *)malloc(nnodes * sizeof(BOOL));
	memset(adj[i], 0, nnodes * sizeof(BOOL));
    }

    for(i=0 ; i<nnodes ; ) {
	x = ((int)nrand48(xsubi)%n);
	y = ((int)nrand48(xsubi)%n);
	if(grid[x][y] == UNKNOWN)
	    grid[x][y] = i++;
    }

    for(i=0, j=0 ; i<nnodes; j++) {
	x	= j%n;
	y	= j/n;
	if(grid[x][y] == UNKNOWN)
	    continue;
	sprintf(chararray, fmt, i);
	add_node(NT_HOST, chararray, TRUE, &wasnew);
	NP[i].nattr.x	= (1.5*x+1) * DEF_NODE_X;
	NP[i].nattr.y	= (1.5*y+1) * DEF_NODE_Y;
	grid[x][y]	= i++;
    }

    minlinks	= (nnodes<6) ? n/2 : (3*nnodes)/2;
    for(i=0 ; i<minlinks ; i++)
	add1link(n, grid, nnodes, adj);
    i=0;
    while(!connected(nnodes, adj)) {
	add1link(n, grid, nnodes, adj);
	++i;
    }
    if(vflag)
	fprintf(stderr,"%d extra link%s required for connectivity\n", P(i));

    for(i=0 ; i<n ; ++i)
	free(grid[i]);
    free(grid);
    for(i=0 ; i<nnodes ; ++i)
	free(adj[i]);
    free(adj);
}


/* --------------------------------------------------------------------- */


void init_reboot_args(NODEATTR *na, int argc, char **argv)
{
    int	n;

    na->reboot_args	= (char **)malloc((argc+1) * sizeof(char *));
    for(n=0 ; n<argc ; ++n)
	na->reboot_args[n]	= strdup(argv[n]);
    na->reboot_args[argc]	= (char *)NULL;
}


void check_topology(int Tflag, int argc, char **argv)
{
    extern int	application_bounds(int *minmsg, int *maxmsg);
    extern int	check_ethernets(BOOL **adj);

    CnetAddr	a;
    NODEATTR	*na;
    int		n, p, nrouters;
    int		x, y, rootn;
    int		appl_minmsg, appl_maxmsg;

    LINK	*lp;
    BOOL	**adj;

    for(n=0, nrouters=0 ; n<_NNODES ; n++) {
	extern char	*random_osname(int randint);

/*  ASSIGN A RANDOM OPERATING SYSTEM TYPE IF NECESSARY */
	if(NP[n].nodetype == NT_ROUTER)
	    ++nrouters;
	else if(NP[n].nattr.osname == (char *)NULL)
	    NP[n].nattr.osname = random_osname((int)nrand48(xsubi));
    }
    n = _NNODES - nrouters;
    if(dflag)
	fprintf(stderr,"%d host%s, %d router%s and %d link%s\n",
				    P(n), P(nrouters), P(_NLINKS) );

/*  ENSURE THAT WE HAVE AT LEAST 2 HOSTS (APPLICATION LAYERS) */
    if(n < 2) {
	fputs("A network must have >= 2 hosts\n",stderr);
	++nerrors;
    }

    application_bounds(&appl_minmsg, &appl_maxmsg);
    for(n=0 ; n<(_NNODES-1) ; n++) {
	int	my_minmsg;
	int	my_maxmsg;

/*  NEXT, CHECK FOR DUPLICATE USER-SPECIFIED NODE ADDRESSES */
	na	= &NP[n].nattr;
	a	= na->address;
	for(p=n+1 ; p<_NNODES ; p++)
	    if(a == NP[p].nattr.address) {
		fprintf(stderr, "%s and %s have the same node address (%u)\n",
				NP[n].nodename, NP[p].nodename, a);
		++nerrors;
	    }

/*  CHECK THAT USER-REQUESTED MESSAGE SIZES ARE NEITHER TOO BIG NOR TOO SMALL */
	my_minmsg	= WHICH(na->minmessagesize, DEFAULTNODE.minmessagesize);
	my_maxmsg	= WHICH(na->maxmessagesize, DEFAULTNODE.maxmessagesize);

	if(my_minmsg < appl_minmsg) {
	    fprintf(stderr,
	    "%s has minmessagesize(=%d) < application layer requirements(=%d)\n",
			    NP[n].nodename, my_minmsg, appl_minmsg);
	    ++nerrors;
	}
	if(my_maxmsg > appl_maxmsg) {
	    fprintf(stderr,
	    "%s has maxmessagesize(=%d) > application layer requirements(=%d)\n",
			    NP[n].nodename, my_maxmsg, appl_maxmsg);
	    ++nerrors;
	}
	if(my_minmsg > my_maxmsg) {
	    fprintf(stderr,"%s has minmessagesize(=%d) > maxmessagesize(=%d)\n",
			    NP[n].nodename, my_minmsg, my_maxmsg);
	    ++nerrors;
	}
    }

/*  ALLOCATE AND INITIALIZE THE ADJACENCY MATIRX (CHECK FOR ETHERNETS TOO) */
    adj		= (BOOL **)malloc(_NNODES * sizeof(BOOL *));
    for(n=0 ; n<_NNODES ; ++n) {
	adj[n]	= (BOOL *)malloc(_NNODES * sizeof(BOOL));
	memset(adj[n], 0, _NNODES * sizeof(BOOL));
    }

    check_ethernets(adj);

/*  IF ANY SIGNIFICANT ERRORS OCCURED, TERMINATE THE PROGRAM */
    if(nerrors)
	cleanup(1);

    rootn	= (int)sqrt((double)(_NNODES-1)) + 1;
    for(n=0 ; n < _NNODES ; n++) {
	na	= &NP[n].nattr;
	x	= n%rootn;
	y	= n/rootn;

/*  GIVE SOME DEFAULT X,Y COORDINATES IF THEY HAVE NOT BEEN SPECIFIED */
	if(na->x == 0)
	    na->x		= (2*x+1) * DEF_NODE_X;
	if(na->y == 0)
	    na->y		= (2*y+1) * DEF_NODE_Y;
	if(na->winx == 0)
	    na->winx		= (5*x+1) * DEF_NODE_X;
	if(na->winy == 0)
	    na->winy		= (5*y+1) * DEF_NODE_Y;

/*  ADD ANY REBOOT ARGUMENTS IF NOT PROVIDED BY TOPOLOGY FILE */
	if(na->reboot_args == (char **)NULL)
	    init_reboot_args(na, argc, argv);
    }

/*  NEXT, PROVIDE A WARNING (ONLY) IF THE TOPOLOGY IS NOT CONNECTED */
    for(n=0, lp=LP ; n<_NLINKS ; n++, lp++)
	if(LP[n].linktype == LT_POINT2POINT) {
	    int	from	= lp->endmin;
	    int	to	= lp->endmax;
	    adj[from][to] = adj[to][from] = TRUE;
	}

    if(!connected(_NNODES, adj))
	fprintf(stderr, "*** warning: this topology is not connected\n");
    for(n=0 ; n<_NNODES ; ++n)
	free(adj[n]);
    free(adj);

/*  IF ATTEMPTING TO DRAWN FRAMES IN A 2-NODE WORLD (ONLY), POSITION NODES */
    if(_NNODES == 2 && gattr.drawframes && nethernets == 0 && !Tflag && Wflag) {
	NP[0].nattr.x		= DEF_NODE_X;
	NP[0].nattr.y		= DEF_NODE_Y;
	NP[1].nattr.x		= DRAWFRAME_WIDTH - DEF_NODE_X;
	NP[1].nattr.y		= DEF_NODE_Y;
    }
    else
	gattr.drawframes	= FALSE;
}


/* -------- Called via cnet -p ...  or the "Save Topology" button -------- */


static	FILE	*topfp;

static void print_link_attr(int always, LINKATTR *cmp, LINKATTR *la, int curly)
{
    char *m;

    if(memcmp((char *)cmp, (char *)la, sizeof(LINKATTR)) == 0)
	return;
    if(always)		m = "";
    else if(curly)	m = "\t";
    else		m = "    ";

    if(curly)
	fputs("{\n",topfp);
    if(always || (la->bandwidth != DEFAULT && 
		  la->bandwidth != cmp->bandwidth))
	fprintf(topfp,"%sbandwidth\t\t= %dbps\n", m,la->bandwidth);
    if(always || (la->costperbyte != DEFAULT && 
		  la->costperbyte != cmp->costperbyte))
	fprintf(topfp,"%scostperbyte\t\t= %d\n",m,la->costperbyte);
    if(always || (la->costperframe != DEFAULT && 
		  la->costperframe != cmp->costperframe))
	fprintf(topfp,"%scostperframe\t\t= %d\n",m,la->costperframe);
    if(always || (la->probframecorrupt != DEFAULT && 
		  la->probframecorrupt != cmp->probframecorrupt))
	fprintf(topfp,"%sprobframecorrupt\t= %d\n",
					m,la->probframecorrupt);
    if(always || (la->probframeloss != DEFAULT && 
		  la->probframeloss != cmp->probframeloss))
	fprintf(topfp,"%sprobframeloss\t\t= %d\n",m,la->probframeloss);
    if(always || (la->transmitbufsize != DEFAULT && 
		  la->transmitbufsize != cmp->transmitbufsize))
	fprintf(topfp,"%stransmitbufsize\t\t= %dbytes\n",
					m,la->transmitbufsize);

    if(always || (int64_NE(la->propagationdelay, DEFAULT64) && 
		  int64_NE(la->propagationdelay, cmp->propagationdelay)))
	fprintf(topfp,"%spropagationdelay\t= %susec\n",
				    m,int64_L2A(la->propagationdelay,0));

    if(always || (int64_NE(la->linkmtbf, DEFAULT64) && 
		  int64_NE(la->linkmtbf, cmp->linkmtbf)))
	fprintf(topfp,"%slinkmtbf\t\t= %susec\n", m,int64_L2A(la->linkmtbf,0));
    if(always || (int64_NE(la->linkmttr, DEFAULT64) && 
		  int64_NE(la->linkmttr, cmp->linkmttr)))
	fprintf(topfp,"%slinkmttr\t\t= %susec\n", m,int64_L2A(la->linkmttr,0));
    if(curly)
	fputs("    }",topfp);
}

static void print_links(int from, int nlinks, int *ls)
{
    int l, t;

    for(l=1 ; l<=nlinks ; l++) {
	t	= ls[l];
	if(LP[t].linktype != LT_POINT2POINT)
	    continue;

	fputs("    ",topfp);
	if(LP[t].endmin == from) {
	    fprintf(topfp,"link to %s ",NP[LP[t].endmax].nodename);
	    print_link_attr(FALSE,&(NP[from].lattr),&(LP[t].lattrmin),TRUE);
	}
	else {
	    fprintf(topfp,"link to %s ",NP[LP[t].endmin].nodename);
	    print_link_attr(FALSE,&(NP[from].lattr),&(LP[t].lattrmax),TRUE);
	}
	fputs("\n",topfp);
    }
}

static void print_node_attr(int always, NODEATTR *na, char *m)
{
    if(always || strcmp(na->compile,DEFAULTNODE.compile) != 0)
	fprintf(topfp,"%scompile\t\t\t= \"%s\"\n",m,na->compile);
    if(always || strcmp(na->reboot_func,DEFAULTNODE.reboot_func) != 0)
	fprintf(topfp,"%srebootfunc\t\t= \"%s\"\n",m,na->reboot_func);

    if(always || na->minmessagesize != DEFAULT)
	fprintf(topfp,"%sminmessagesize\t\t= %dbytes\n",
						m,na->minmessagesize);
    if(always || na->maxmessagesize != DEFAULT)
	fprintf(topfp,"%smaxmessagesize\t\t= %dbytes\n",
						m,na->maxmessagesize);

    if(always || int64_NE(na->messagerate, DEFAULT64))
       fprintf(topfp,"%smessagerate\t\t= %susec\n",
						m,int64_L2A(na->messagerate,0));

    if(always || int64_NE(na->nodemtbf, DEFAULT64))
	fprintf(topfp,"%snodemtbf\t\t= %susec\n", m,int64_L2A(na->nodemtbf,0));

    if(always || int64_NE(na->nodemttr, DEFAULT64))
	fprintf(topfp,"%snodemttr\t\t= %susec\n", m,int64_L2A(na->nodemttr,0));
}

void save_topology(char *filenm)
{
    extern void	print_ethernets(FILE *fp);

    NODE	*np;
    int		n;

    if(filenm == (char *)NULL || *filenm == '\0')
	topfp	= stdout;
    else
	if((topfp = fopen(filenm, "w")) == (FILE *)NULL) {
	    fprintf(stderr,"%s: cannot create %s\n", argv0,filenm);
	    return;
	}

/*  FIRSTLY, PRINT THE DEFAULT NODE AND LINK ATTRIBUTES */
    print_node_attr(TRUE, &DEFAULTNODE, "");
    print_link_attr(TRUE, (LINKATTR *)chararray, &DEFAULTLINK, FALSE);
    fprintf(topfp,"\n");

/*  NEXT, PRINT ANY GLOBAL ATTRIBUTES THAT HAVE BEEN DEFINED */
    if(gattr.bgimage)
	fprintf(topfp,"bgimage = \"%s\"\n", gattr.bgimage);
    if(gattr.drawframes)
	fprintf(topfp,"drawframes = true\n");
    if(gattr.trace_filenm)
	fprintf(topfp,"tracefile = \"%s\"\n", gattr.trace_filenm);
    if(gattr.showcostperframe)
	fprintf(topfp,"showcostperframe = true\n");
    else if(gattr.showcostperbyte)
	fprintf(topfp,"showcostperbyte = true\n");
    fprintf(topfp,"\n");

/*  NEXT, PRINT ANY ETHERNET SEGMENTS THAT WE HAVE */
    print_ethernets(topfp);

/*  FINALLY, FOR EACH NODE, PRINT ITS LOCAL NODE AND LINK INFORMATION */
    for(n=0, np=NP ; n<_NNODES ; n++, np++) {
	fprintf(topfp,"%s %s {\n",
		np->nodetype == NT_HOST ? "host" : "router", np->nodename);
	fprintf(topfp,"    x=%d, y=%d\n", np->nattr.x,np->nattr.y);
	if(np->nattr.osname != (char *)NULL)
	    fprintf(topfp,"    osname = \"%s\"\n", np->nattr.osname);

	print_node_attr(FALSE, &(np->nattr), "    ");
	print_link_attr(FALSE, &DEFAULTLINK, &(np->lattr), FALSE);
	print_links(n, np->nlinks, np->links);
	fprintf(topfp,"}\n");
    }
    if(topfp != stdout)
	fclose(topfp);
}

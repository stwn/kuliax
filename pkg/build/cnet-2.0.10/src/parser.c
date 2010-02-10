#include "cnetheader.h"
#include "lexical.h"
#include <stdarg.h>

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

extern	void init_reboot_args(NODEATTR *na, int argc, char **argv);

#define expect(t,msg)	if(token == t) gettoken(); \
			else compile_error("%s expected\n", msg)


/* ----------------------- Compile time errors ------------------------- */

static void point_to_error()
{
    int	i;

    fputs(input.line,stderr);
    for(i=0 ; i<input.cc ; ++i)
	if(input.line[i] == '\t')
	     fputc('\t',stderr);
	else fputc(' ',stderr);
    fprintf(stderr,"^\n%s:%d: ", input.name,input.lc);
}

static void compile_error(const char *fmt, ...)
{
#if	defined(__GNUC__)
    extern int	vfprintf(FILE *, const char *, va_list);
#endif

    va_list	ap;

    point_to_error();
    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);
    va_end(ap);
    cleanup(1);
}

/* ------------------- check all required data types -------------------- */

static void check_integer(int *i)
{
    if(token != T_INTCONST)
	compile_error("integer constant expected\n"); 
    *i = input.value;
    gettoken();
}

static void check_positive(int *i)
{
    if(token != T_INTCONST || input.value <= 0)
	compile_error("positive integer constant expected\n"); 
    *i = input.value;
    gettoken();
}

static void check_boolean(int *bool)
{
    if(token == T_TRUE)
	*bool = TRUE;
    else if(token == T_FALSE)
	*bool = FALSE;
    else if(token == T_TOGGLE)
	*bool = !(*bool);
    else
	compile_error("boolean attribute expected\n"); 
    gettoken();
}

static void check_osname(NODEATTR *na)
{
    if(token != T_STRCONST)
	compile_error("operating system name expected\n"); 
    else
	na->osname	= strdup(chararray);
    gettoken();
}

static void check_string(char **strp)
{
    if(token != T_STRCONST)
	compile_error("string constant expected\n"); 
    else
	*strp = strdup(chararray);
    gettoken();
}

static void check_time_scale(CnetInt64 *value)
{
/*  UNITS OF ALL TIMES ARE MICROSECONDS */

    CnetInt64	value64, scale;

    if(token != T_INTCONST || input.value < 0)
	compile_error("time constant expected\n"); 
    int64_I2L(value64, input.value);
    gettoken();

    switch ((int)token) {
	case T_SEC :	int64_MUL(value64, value64, MILLION64);
			break;
	case T_MSEC :	int64_I2L(scale, 1000);
			int64_MUL(value64, value64, scale);
			break;
	case T_USEC :	
			break;
	default :	compile_error("'s', 'ms' or 'us' expected\n");
			break;
    }
    gettoken();
    *value	= value64;
}

#if	defined(NOT_NEEDED)
static void check_float(char **p)
				/* note: floats are kept as strings and not */
{				/* converted to an internal representation */
    if(token != T_REALCONST)
	compile_error("floating point constant expected\n"); 
    else
	*p = strdup(chararray);
    gettoken();
}
#endif

/* ---------------------------------------------------------------------- */


static void scale_bandwidth(int *value)
{
/*  UNITS OF nodeattr.bandwidth ARE BITS PER SECOND */

    switch ((int)token) {
	case T_BITSPS :							break;
	case T_KBITSPS :	*value *= 1000;				break;
	case T_MBITSPS :	*value *= (1000*1000);			break;
	case T_BYTESPS :	*value *= 8;				break;
	case T_KBYTESPS :	*value *= (8*1000);			break;
	case T_MBYTESPS :	*value *= (8*1000*1000);		break;
	default :		compile_error("bandwidth units expected\n");
									break;
    }
    if(*value <= 0)		/* attempt to fix overflow */
	*value	= int32_MAXINT;
    gettoken();
}

static void scale_bytes(int *value)
{
/*  UNITS OF nodeattr.*messagesize ARE BYTES */

    switch ((int)token) {
	case T_BYTES :		gettoken();				break;
	case T_KBYTES :		gettoken(); *value *= 1024;		break;
	case T_MBYTES :		gettoken(); *value *= (1024*1024);	break;
	default :		/* maybe OK */				break;
    }
    if(*value <= 0)		/* attempt to fix overflow */
	*value	= int32_MAXINT;
}


/* ---------------------------------------------------------------------- */

static void node_attr(TOKEN attr, NODEATTR *na)
{
    gettoken();
    expect(T_EQ,"'='");

    switch ((int)attr) {
    case T_ATTR_NODE_ADDRESS :
	check_integer((int *)&(na->address));
	break;
    case T_ATTR_NODE_COMPILE :
	check_string(&(na->compile));
	break;
    case T_ATTR_NODE_MAXMESSAGESIZE :
	check_positive(&(na->maxmessagesize));
	scale_bytes(&(na->maxmessagesize));
	if(na->maxmessagesize > MAX_MESSAGE_SIZE)
	compile_error( "maxmessagesize exceeds MAX_MESSAGE_SIZE (=%d bytes)\n",
		MAX_MESSAGE_SIZE);
	break;
    case T_ATTR_NODE_MESSAGERATE :
	check_time_scale(&(na->messagerate));
	break;
    case T_ATTR_NODE_MINMESSAGESIZE :
	check_positive(&(na->minmessagesize));
	scale_bytes(&(na->minmessagesize));
	break;
    case T_ATTR_NODE_MTBF :
	check_time_scale(&(na->nodemtbf));
	break;
    case T_ATTR_NODE_MTTR :
	check_time_scale(&(na->nodemttr));
	break;
    case T_ATTR_NODE_OSNAME :
	check_osname(na);
	break;
    case T_ATTR_NODE_OUTPUTFILE :
	check_string(&(na->outputfile));
	break;
    case T_ATTR_NODE_REBOOTARGS : {
	int	argc;
	char	**argv;
	char	*str, *s, *t, ch;

	check_string((char **)&str);
	argc	= 0;
	argv	= (char **)malloc((strlen(str)/2+2) * sizeof(char *));

	t = str;
	while(*t) {
	    while(*t == ' ' || *t == '\n')
		++t;
	    if(*t == '\0')
		break;
	    s	= t;
	    while(*t && (*t != ' ' && *t != '\t'))
		++t;
	    ch	= *t;
	    *t	= '\0';
	    argv[argc++]	= strdup(s);
	    *t	= ch;
	}
	argv[argc]	= (char *)NULL;

	init_reboot_args(na, argc, argv);
	free(argv);
	free(str);
	break;
    }
    case T_ATTR_NODE_REBOOTFUNC :
	check_string(&(na->reboot_func));
	break;
    case T_ATTR_NODE_TRACE :
	check_boolean(&(na->trace_all));
	break;
    case T_ATTR_NODE_WINOPEN :
	check_boolean(&(na->winopen));
	break;
    case T_ATTR_NODE_WINX :
	check_integer(&(na->winx));
	break;
    case T_ATTR_NODE_WINY :
	check_integer(&(na->winy));
	break;
    case T_ATTR_NODE_X :
	check_integer(&(na->x));
	break;
    case T_ATTR_NODE_Y :
	check_integer(&(na->y));
	break;
    }
}

static void link_attr(TOKEN attr, LINKATTR *lp)
{
    gettoken();
    expect(T_EQ,"'='");

    switch ((int)attr) {
    case T_ATTR_LINK_BANDWIDTH :
	check_positive((int *)&(lp->bandwidth));
	scale_bandwidth((int *)&(lp->bandwidth));
	break;
    case T_ATTR_LINK_COSTPERBYTE :
	check_integer(&(lp->costperbyte));
	if(token == T_C) gettoken();
	break;
    case T_ATTR_LINK_COSTPERFRAME :
	check_integer(&(lp->costperframe));
	if(token == T_C) gettoken();
	break;
    case T_ATTR_LINK_PROBFRAMECORRUPT :
	check_integer(&(lp->probframecorrupt));
	break;
    case T_ATTR_LINK_PROBFRAMELOSS :
	check_integer(&(lp->probframeloss));
	break;
    case T_ATTR_LINK_PROPAGATIONDELAY :
	check_time_scale(&(lp->propagationdelay));
	break;
    case T_ATTR_LINK_TRANSMITBUFSIZE :
	check_positive(&(lp->transmitbufsize));
	scale_bytes(&(lp->transmitbufsize));
	break;
    case T_ATTR_LINK_MTBF :
	check_time_scale(&(lp->linkmtbf));
	break;
    case T_ATTR_LINK_MTTR :
	check_time_scale(&(lp->linkmttr));
	break;
    }
}

static void parse_node_attrs(NODEATTR *na, LINKATTR *lp)
{
    for(;;) {
	if(is_node_attr(token))
	    node_attr(token,na);
	else if(is_link_attr(token))
	    link_attr(token,lp);
	else if(token == T_COMMA)
	    gettoken();
	else
	    break;
    }
}

static void parse_link_attrs(LINKATTR *lp)
{
    for(;;) {
	if(is_link_attr(token))
	    link_attr(token,lp);
	else if(token == T_COMMA)
	    gettoken();
	else
	    break;
    }
    expect(T_RCURLY,"'}'");
}


/* ---------------------------------------------------------------------- */

static FILE *preprocess_topology(char *filenm, char **defines)
{
#if defined(USE_WIN32)
    return( fopen(filenm, "r") );

#else
    extern FILE	*fdopen(int, const char *);

    char	*av[64];		/* hoping that this is enough */
    char	*cpp;
    int		ac=0, p[2];

    if(pipe(p) == -1) {
	fprintf(stderr,"%s : cannot create pipe\n",argv0);
	cleanup(1);
    }

    switch (fork()) {
    case -1 :	fprintf(stderr,"%s : cannot fork\n",argv0);
		cleanup(1);

/*	$cpp -C [file] (in child) | parse() (in parent)
 */
    case 0  :	close(p[0]);			/* child */
		dup2(p[1],1);
		close(p[1]);

#if	defined(CPP)
		cpp			= findenv("CNETCPP", CNETCPP);
		av[ac++]		= "cpp";
#else
#if	USE_GCC_COMPILER
		cpp			= findenv("CNETGCC", CNETGCC);
		av[ac++]		= "gcc";
#else
		cpp			= findenv("CNETCC", CNETCC);
		av[ac++]		= "cc";
#endif
		av[ac++]		= "-E";
		av[ac++]		= "-x";
		av[ac++]		= "c-header";
#endif
		av[ac++]		= OS_DEFINE;
		if(defines)
		    while(*defines)		/* add any -D.. switches */
			av[ac++]	= *defines++;
		av[ac++]		= filenm;
		av[ac]			= (char *)NULL;

		if(dflag) {
		    fprintf(stderr,"%s ", cpp);
		    for(ac=1 ; av[ac] ; ++ac)
			fprintf(stderr,"%s ",av[ac]);
		    fputc('\n', stderr);
		}

		execv(cpp, av);
		fprintf(stderr,"%s : cannot exec %s\n", argv0,cpp);
		cleanup(1);
		break;
    default  :	close(p[1]);			/* parent */
		return fdopen(p[0],"r");
		break;
    }
#endif
    return ((FILE *)NULL);
}

static int node_defn(CnetNodetype nodetype)
{
    extern int	add_node(CnetNodetype, char *, int, int *);

    int		thisnode, wasnew;

    if(token == T_NAME) {
	thisnode	= add_node(nodetype, chararray, TRUE, &wasnew);
	gettoken();
    }
    else {
	compile_error("node name expected\n");
	thisnode	= 0;
    }
    expect(T_LCURLY,"'{'");

    for(;;) {
	if(is_node_attr(token) || is_link_attr(token))	/* node-wide attrs */
	    parse_node_attrs(&(NP[thisnode].nattr), &(NP[thisnode].lattr));
	else if(is_compass_direction(token)) {
	    int	dx=0, dy=0;

	    while(is_compass_direction(token)) {
		switch ((int)token) {
		case T_NORTH	: --dy;		break;
		case T_NORTHEAST: --dy;	++dx;	break;
		case T_EAST	: 	++dx;	break;
		case T_SOUTHEAST: ++dy;	++dx;	break;
		case T_SOUTH	: ++dy;		break;
		case T_SOUTHWEST: ++dy;	--dx;	break;
		case T_WEST	: 	--dx;	break;
		case T_NORTHWEST: --dy;	--dx;	break;
		}
		gettoken();
		if(token == T_INTCONST) {
		    dx *= input.value; dy *= input.value;
		    gettoken();
		}
	    }
	    expect(T_OF,"'of' or compass direction");
	    if(token == T_NAME) {
		int	othernode;

		othernode = add_node(NT_HOST,chararray,FALSE,&wasnew);
		NP[thisnode].nattr.x = NP[othernode].nattr.x + 2*dx*DEF_NODE_X;
		NP[thisnode].nattr.y = NP[othernode].nattr.y + 2*dy*DEF_NODE_Y;
		gettoken();
	    }
	    else
		compile_error("node name expected\n");
	    if(token == T_COMMA)
		gettoken();
	}
	else
	    break;
    }
    while(token == T_LINK) {
	int	othernode	= 0;
	int	thislink	= 0;

	gettoken();
	expect(T_TO,"'to'");
	if(token == T_NAME) {
	    othernode	= add_node(NT_HOST, chararray, FALSE, &wasnew);
	    if(thisnode == othernode)
		compile_error("link to oneself is invalid\n");
	    else {
		extern int add_link(CnetLinktype, int, int, CnetNicaddr);

		thislink	= add_link(LT_POINT2POINT, thisnode, othernode,
					  NULL);
		if(thisnode < othernode)
		    LP[thislink].lattrmin	= NP[thisnode].lattr;
		else
		    LP[thislink].lattrmax	= NP[thisnode].lattr;
	    }
	    gettoken();
	}
	else
	    compile_error("name of node at other end of link expected\n");
	if(token == T_LCURLY) {
	    gettoken();

	    if(thisnode < othernode)
		parse_link_attrs(&(LP[thislink].lattrmin));
	    else
		parse_link_attrs(&(LP[thislink].lattrmax));
	}
	if(token == T_COMMA)
	    gettoken();
    }
    expect(T_RCURLY,"'}'");
    return(thisnode);
}


static void ethernet_defn()
{
    extern int	new_ethernet(char *name);
    extern void	extend_ethernet(int ethern, int whichnode, CnetNicaddr);

    ETHERNET		*ep;
    CnetNicaddr		nicaddr;
    char		*ethername;
    int			thisether, thisnode;

    if(token != T_NAME)
	compile_error("ethernet name expected\n");
    ethername	= strdup(chararray);
    gettoken();

    expect(T_LCURLY,"'{'");

    thisether	= new_ethernet(ethername);
    free(ethername);

    ep		= &ethernets[thisether];
    while(token != T_RCURLY) {
	if(token == T_ATTR_NODE_X) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_positive(&(ep->x));
	    continue;
	}
	if(token == T_ATTR_NODE_Y) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_positive(&(ep->y));
	    continue;
	}
	if(token == T_COMMA) {
	    gettoken();
	    continue;
	}

	if(token == T_NICADDR) {
	    gettoken();
	    if(token == T_EQ)
		gettoken();
	}
	if(token != T_ETHERADDR)
	    compile_error("ethernet address expected\n");

	memcpy(nicaddr, input.nicaddr, LEN_NICADDR);
	gettoken();
	if(token == T_HOST || token == T_ROUTER) {
	    TOKEN	cptoken	= token;

	    gettoken();
	    thisnode	= node_defn(cptoken == T_HOST ? NT_HOST : NT_ROUTER);
	    extend_ethernet(thisether, thisnode, nicaddr);
	}
	else
	    compile_error("host or router definition expected\n");
    }
    expect(T_RCURLY,"'}'");
}

void parse_topology(FILE *fp, char *filenm, char **defines)
{
    int		run_cpp = (defines[0] != (char *)NULL);
    TOKEN	cptoken;

    while(run_cpp == FALSE) {		/* save an unnecessary fork/exec */
	fgets(chararray,BUFSIZ,fp);
	if(feof(fp))
	    break;
	if(*chararray == '#')		/* a cpp control line */
	    run_cpp	= TRUE;
    }
    if(run_cpp) {
	fclose(fp);
	input.fp = preprocess_topology(filenm, defines);
    }
    else {
	rewind(fp);
	input.fp = fp;
    }
    input.cc	= 0;
    input.ll	= 0;
    input.lc	= 0;
    input.name	= filenm;

    gettoken();
    while(token != T_EOF) {

	if(is_node_attr(token) || is_link_attr(token))    /* topology-wide */
	    parse_node_attrs(&DEFAULTNODE, &DEFAULTLINK);

	else if(token == T_HOSTTYPE || token == T_ROUTERTYPE) {
	    extern void add_nodetype(CnetNodetype, char *,
				      NODEATTR **, LINKATTR **);
	    NODEATTR	*na;
	    LINKATTR	*la;

	    cptoken	= token;
	    gettoken();
	    if(token == T_NAME) {
		add_nodetype(cptoken==T_HOSTTYPE ? NT_HOST : NT_ROUTER,
			     chararray, &na, &la);
		gettoken();
	    }
	    else
		compile_error("hosttype or routertype name expected\n");

	    expect(T_LCURLY,"'{'");
	    if(is_node_attr(token))
		parse_node_attrs(na, la);
	    expect(T_RCURLY,"'}'");
	}
	else if(token == T_LINKTYPE) {
	    extern void	add_linktype(CnetLinktype, char *, LINKATTR **);
	    LINKATTR	*la;

	    gettoken();
	    if(token == T_NAME) {
		add_linktype(LT_POINT2POINT, chararray, &la);
		gettoken();
	    }
	    else
		compile_error("linktype name expected\n");

	    expect(T_LCURLY,"'{'");
	    if(is_link_attr(token))
		parse_link_attrs(la);
	    expect(T_RCURLY,"'}'");
	}
	else if(token == T_HOST || token == T_ROUTER) {
	    cptoken	= token;
	    gettoken();
	    (void)node_defn(cptoken == T_HOST ? NT_HOST : NT_ROUTER);
	}
	else if(token == T_ETHERNET) {
	    gettoken();
	    ethernet_defn();
	}
	else if(token == T_DRAWFRAMES) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_boolean(&(gattr.drawframes));
	}
	else if(token == T_TRACEFILE) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_string(&gattr.trace_filenm);
	}
	else if(token == T_BGIMAGE) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_string(&gattr.bgimage);
	}
	else if(token == T_SHOWCOSTPERFRAME) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_boolean(&(gattr.showcostperframe));
	}
	else if(token == T_SHOWCOSTPERBYTE) {
	    gettoken();
	    expect(T_EQ,"'='");
	    check_boolean(&(gattr.showcostperbyte));
	}
	else if(token == T_BAD)
	    compile_error("unknown symbol or premature end of file\n");

	else if(!feof(input.fp)) {
	    compile_error("host or router definition expected\n");
	    gettoken();
	}
    }				/* while(!feof(input.fp)) */
    fclose(input.fp);
}

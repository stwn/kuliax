#include "cnetheader.h"

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

int CNET_set_debug_string(CnetEvent ev, char *str)
{
    if(str == (char *)NULL)
	str = "";
    if((int)ev<(int)EV_DEBUG1 || (int)ev >= ((int)EV_DEBUG1 + N_CNET_DEBUGS)) {
	if(gattr.trace_events)
	    TRACE("\tCNET_set_debug_string(ev=%d,\"%s\") = -1 %s\n",
				(int)ev,str,cnet_errname[(int)ER_BADEVENT]);
	cnet_errno	= ER_BADEVENT;
	return (-1);
    }
    if(gattr.trace_events)
	TRACE("\tCNET_set_debug_string(%s,\"%s\") = 0\n",
				cnet_evname[(int)ev],str);

#if	defined(USE_TCLTK)

    if(Wflag) {
#define	DEBUG_STRING_WIDTH	12
	int	d = (int)ev-(int)EV_DEBUG1;

	if(NP[THISNODE].debug_str[d])
	    free(NP[THISNODE].debug_str[d]);

	if(*str == '\0')
	    NP[THISNODE].debug_str[d]	= strdup("");
	else {
	    strncpy(chararray,str,DEBUG_STRING_WIDTH);
	    chararray[DEBUG_STRING_WIDTH]	= '\0';
	    NP[THISNODE].debug_str[d]	= strdup(chararray);
	}

	str = NP[THISNODE].debug_str[d];
	TCLTK("SetDebugString .node%d.d.debug%d \"%s\"",
		THISNODE, d, (str == (char *)NULL || *str == '\0') ? " " : str);
#undef	DEBUG_STRING_WIDTH
    }

#endif	/* defined(USE_TCLTK) */

    return 0;
}

/* ------------------ ONLY CODE FOR USE_TCLTK --------------------- */

#if	defined(USE_TCLTK)

#define	N_NODE_CHOICES		 4

static struct {
    char	*title;
    char	*keyword;
    char	*labels[N_NODE_CHOICES];
    CnetInt64	   orig[N_NODE_CHOICES];
    CnetInt64	 values[N_NODE_CHOICES];
    int		 panel_value;
} node_defaults[3];


#define	N_NODE_DEFAULTS		(sizeof(node_defaults)/sizeof(node_defaults[0]))

#define	MSGRATE			0
#define	MINSIZE			1
#define	MAXSIZE			2

typedef	struct	{
    int		n;
    int		r;
    int		c;
    CnetInt64	value;
} NODECHOICE;


static void init_node_defaults()
{
    int	n;

    node_defaults[MSGRATE].title		= "Message rate :";
    node_defaults[MSGRATE].keyword	= "messagerate";
    for(n=0 ; n<N_NODE_DEFAULTS ; ++n) {
	node_defaults[MSGRATE].labels[n]	= NULL;
	node_defaults[MSGRATE].values[n]	= int64_ZERO;
    }
    int64_I2L(node_defaults[MSGRATE].orig[0],	 200000);
    int64_I2L(node_defaults[MSGRATE].orig[1],	  10000);
    int64_I2L(node_defaults[MSGRATE].orig[2],	 500000);
    int64_I2L(node_defaults[MSGRATE].orig[3],	5000000);

    node_defaults[MINSIZE].title		= "Min. message size :";
    node_defaults[MINSIZE].keyword	= "minmessagesize";
    for(n=0 ; n<N_NODE_DEFAULTS ; ++n) {
	node_defaults[MINSIZE].labels[n]	= NULL;
	node_defaults[MINSIZE].values[n]	= int64_ZERO;
    }
    int64_I2L(node_defaults[MINSIZE].orig[0],	 100);
    int64_I2L(node_defaults[MINSIZE].orig[1],	  32);
    int64_I2L(node_defaults[MINSIZE].orig[2],	 256);
    int64_I2L(node_defaults[MINSIZE].orig[3],	   K);

    node_defaults[MAXSIZE].title		= "Max. message size :";
    node_defaults[MAXSIZE].keyword	= "maxmessagesize";
    for(n=0 ; n<N_NODE_DEFAULTS ; ++n) {
	node_defaults[MAXSIZE].labels[n]	= NULL;
	node_defaults[MAXSIZE].values[n]	= int64_ZERO;
    }
    int64_I2L(node_defaults[MAXSIZE].orig[0],	  K);
    int64_I2L(node_defaults[MAXSIZE].orig[1],	2*K);
    int64_I2L(node_defaults[MAXSIZE].orig[2],	4*K);
    int64_I2L(node_defaults[MAXSIZE].orig[3],	MAX_MESSAGE_SIZE);
    node_defaults[MAXSIZE].panel_value	= 0;
}


/* ---------------------------------------------------------------------- */


static int node_debug_button(ClientData data, Tcl_Interp *interp,
				int argc, char *argv[])
{
    extern void	single_event(int, CnetEvent, CnetTimer);
    CnetEvent	ev;
    int		d, n;

    if(vflag) {
	for(n=0 ; n<argc-1 ; ++n)
	    fprintf(stderr,"%s ", argv[n]);
	fprintf(stderr,"%s\n", argv[argc-1]);
    }
    if(argc != 3) {
	interp->result	= "wrong # args";
	return TCL_ERROR;
    }
    n	= atoi(argv[1]);
    d	= atoi(argv[2]);
    if(n < 0 || n >= _NNODES || d < 0 || d >= N_CNET_DEBUGS) {
	interp->result	= "invalid node or debug #";
	return TCL_ERROR;
    }

    ev	= (CnetEvent)((int)EV_DEBUG1+d);
    if(vflag)
	fprintf(stderr,"%s.%s\n", NP[n].nodename, cnet_evname[(int)ev]);

    if(cnet_state == STATE_PAUSED)
	single_event(n, ev, NULLTIMER);
    else
	schedule_event(ev, n, int64_ONE, NULLTIMER, NP[n].data[(int)ev]);

    return TCL_OK;
}


static void fmt_choice(CnetInt64 value, int div, char *shortfmt, char *longfmt)
{
    CnetInt64	tmp64;

    int64_I2L(tmp64, div);
    int64_MOD(tmp64, value, tmp64);
    if(int64_IS_ZERO(tmp64)) {
	int64_I2L(tmp64, div);
	int64_DIV(value, value, tmp64);
	sprintf(chararray, shortfmt, int64_L2A(value,0));
    }
    else
	sprintf(chararray, longfmt, int64_L2A(value,0));
}

void format_node_defaults(int n)
{
    static int	first_time	= TRUE;
    int		c, d;
    char	cmdbuf[BUFSIZ], *p;

    NODEATTR	*na;
    int		first, i, from;

    if(first_time)  {
	static int	default_displayed = FALSE;	/* never used */
	int		nn;

	init_node_defaults();

	for(d=0 ; d<N_NODE_DEFAULTS ; ++d) {
	    sprintf(chararray,"node_choice_title(%d)",d);
	    Tcl_SetVar(tcl_interp, chararray,
			    node_defaults[d].title, TCL_GLOBAL_ONLY);
	}

	for(nn=DEFAULT ; nn<=0 ; ++nn) {  /* nn just takes on DEFAULT and 0 */
	    char	buf[16];

	    for(d=0 ; d<N_NODE_DEFAULTS ; ++d) {
		sprintf(chararray,"node_choice_set(%d,%d)",nn,d);
		sprintf(buf      ,"%d", node_defaults[d].panel_value);
		Tcl_SetVar(tcl_interp, chararray, buf, TCL_GLOBAL_ONLY);
	    }
	}
	TCLTK_createcommand("node_debug_button", node_debug_button);

	strcpy(chararray, "stdio_quiet(default)");
	Tcl_LinkVar(tcl_interp, chararray,
			    (char *)&DEFAULTNODE.stdio_quiet, TCL_LINK_BOOLEAN);
	strcpy(chararray, "trace_events(default)");
	Tcl_LinkVar(tcl_interp, chararray,
			    (char *)&DEFAULTNODE.trace_all, TCL_LINK_BOOLEAN);
	strcpy(chararray, "node_displayed(default)");
	Tcl_LinkVar(tcl_interp, chararray,
			    (char *)&default_displayed, TCL_LINK_BOOLEAN);

	first_time = FALSE;
    }

    if(n == DEFAULT) {
	na	= &DEFAULTNODE;
	first	= 0;
	node_defaults[MSGRATE].values[0]	= na->messagerate;
	int64_I2L(node_defaults[MINSIZE].values[0], na->minmessagesize);
	int64_I2L(node_defaults[MAXSIZE].values[0], na->minmessagesize);

	node_defaults[MSGRATE].panel_value	=
	node_defaults[MINSIZE].panel_value	=
	node_defaults[MAXSIZE].panel_value	= 0;
    }
    else {
	na	= &(NP[n].nattr);
	first	= 1;
	for(i=0 ; i<N_NODE_DEFAULTS ; i++)
	    node_defaults[i].labels[0]	= strdup("Default");

	if(int64_EQ(na->messagerate, DEFAULT64))
	    node_defaults[MSGRATE].values[1] = node_defaults[MSGRATE].orig[1];
	else
	    node_defaults[MSGRATE].values[1] = na->messagerate;
	
	if(na->minmessagesize == DEFAULT)
	    node_defaults[MINSIZE].values[1] = node_defaults[MINSIZE].orig[1];
	else
	    int64_I2L(node_defaults[MINSIZE].values[1], na->minmessagesize);

	if(na->maxmessagesize == DEFAULT)
	    node_defaults[MAXSIZE].values[1] = node_defaults[MAXSIZE].orig[1];
	else
	    int64_I2L(node_defaults[MAXSIZE].values[1], na->maxmessagesize);

	node_defaults[MSGRATE].panel_value =
				int64_EQ(na->messagerate, DEFAULT64) ? 0 : 1;
	node_defaults[MINSIZE].panel_value =
				(na->minmessagesize == DEFAULT) ? 0 : 1;
	node_defaults[MAXSIZE].panel_value =
				(na->maxmessagesize == DEFAULT) ? 0 : 1;
    }
    for(from=0 ; from<N_NODE_DEFAULTS ; from++)
	for(i=(first+1) ; i<N_NODE_CHOICES ; i++)
	    node_defaults[from].values[i]	= node_defaults[from].orig[i];

    for(i=first ; i<N_NODE_CHOICES ; i++) {
	fmt_choice(node_defaults[MSGRATE].values[i], 1000000, "%ss", "%sus");
	node_defaults[MSGRATE].labels[i]	= strdup(chararray);
    }
    for(i=first ; i<N_NODE_CHOICES ; i++) {
	fmt_choice(node_defaults[MINSIZE].values[i], K, "%sKB", "%sBytes");
	node_defaults[MINSIZE].labels[i]	= strdup(chararray);
    }
    for(i=first ; i<N_NODE_CHOICES ; i++) {
	fmt_choice(node_defaults[MAXSIZE].values[i], K, "%sKB", "%sBytes");
	node_defaults[MAXSIZE].labels[i]	= strdup(chararray);
    }

    if(n == DEFAULT)
	sprintf(cmdbuf, "InitNode default default default .%s 0 0 {", argv0);
    else
	sprintf(cmdbuf, "InitNode %d %s %s .%s %d %d {",
		    n, NP[n].nodename,
		    NP[n].nodetype == NT_HOST ? "host" : "router", 
		    argv0,
		    NP[n].nattr.winx, NP[n].nattr.winy);
    p	= cmdbuf;
    while(*p)
	++p;

    for(d=0 ; d<N_NODE_DEFAULTS ; d++)
	for(c=0 ; c<N_NODE_CHOICES ; c++) {
	    *p++ = ' ';
	    strcpy(p, node_defaults[d].labels[c]);
	    while(*p)
		++p;
	}

    *p++	= '}';
    *p		= '\0';
    TCLTK(cmdbuf);
}

void init_nodewindow(int n)
{
    NODE	*np	= &NP[n];

    sprintf(chararray, "stdio_quiet(%d)", n);
    Tcl_LinkVar(tcl_interp, chararray,
		    (char *)&np->nattr.stdio_quiet, TCL_LINK_BOOLEAN);
    sprintf(chararray, "trace_events(%d)", n);
    Tcl_LinkVar(tcl_interp, chararray,
		    (char *)&np->nattr.trace_all, TCL_LINK_BOOLEAN);

    sprintf(chararray, "node_displayed(%d)", n);
    Tcl_LinkVar(tcl_interp, chararray,
		    (char *)&np->displayed, TCL_LINK_BOOLEAN);

    format_node_defaults(n);
}

void toggle_node_window(int n)
{
    TCLTK("ToggleNodeAttributes %d", n);	/* never default */
}


/* ---------------------------------------------------------------------- */

int node_choice(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    NODECHOICE	nc;
    NODEATTR	*na;
    int		n;

    if(vflag) {
	for(n=0 ; n<argc-1 ; ++n)
	    fprintf(stderr,"%s ", argv[n]);
	fprintf(stderr,"%s\n", argv[argc-1]);
    }
    if(argc != 4) {
	interp->result	= "wrong # args";
	return TCL_ERROR;
    }
    nc.n	= atoi(argv[1]);
    nc.r	= atoi(argv[2]);
    nc.c	= atoi(argv[3]);
    if(nc.n < -1 || nc.n >= _NNODES || nc.c < 0 || nc.c >= N_NODE_CHOICES) {
	interp->result	= "invalid node or choice #";
	return TCL_ERROR;
    }
    nc.value   =
        (nc.c == 0 && nc.n != DEFAULT) ? DEFAULT64
                                       : node_defaults[nc.r].values[nc.c];

    na	= (nc.n==DEFAULT) ? &DEFAULTNODE : (&NP[nc.n].nattr);

    switch (nc.r) {
    case MSGRATE:
	na->messagerate	= nc.value;
	break;
    case MINSIZE:
	int64_L2I(na->minmessagesize, nc.value);
	break;
    case MAXSIZE:
	int64_L2I(na->maxmessagesize, nc.value);
	break;
    }

    return TCL_OK;
}
#endif	/* defined(USE_TCLTK) */

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

/* ------------------ NO CODE IN HERE FOR USE_ASCII --------------------- */

#if	defined(USE_TCLTK)

static struct {
    char	*label;
    char	*keyword;
    int		max;
    int		value;
} sliders[] = {
    { "Cost(per byte) ",		"costperbyte",		 20,	0 },
    { "Cost(per frame) ",		"costperframe",		 20,	0 },
    { "Prob(frame corruption) 1:2^N",	"probframecorrupt",	 10,	0 },
    { "Prob(frame loss) 1:2^N",		"probframeloss",	 10,	0 },
};

#define	N_LINK_SLIDERS	(sizeof(sliders) / sizeof(sliders[0]))

#define	CPB		0
#define	CPF		1
#define	PFC		2
#define	PFL		3


static LINKARG *make_linkarg(int from, int to)
{
    LINKARG	*larg	= NEW(LINKARG);
    LINK	*lp;

    if(from == DEFAULT) {
	larg->from	= DEFAULT;
	larg->to	= DEFAULT;
	larg->la	= &DEFAULTLINK;
    }
    else {
	int	l;
	int	min, max;

	if(from < to)
	    min = from, max = to;
	else
	    min = to  , max = from;

	for(l=0, lp=LP ; l<_NLINKS ; l++, lp++)
	    if( lp->linktype == LT_POINT2POINT &&
		lp->endmin == min && lp->endmax == max) {

		larg->from	= from;
		larg->to	= to;
		larg->link	= lp;
		if(from < to) {
		    larg->la	= &lp->lattrmin;
		    larg->lp	= &lp->panelmin;
		}
		else {
		    larg->la	= &lp->lattrmax;
		    larg->lp	= &lp->panelmax;
		}
		break;
	    }
    }
    return(larg);
}


/* --------------------------------------------------------------------- */


static void link_slider_proc(LINKARG *larg)
{
    int	now = 0;

    switch (larg->n) {
    case CPB:	larg->la->costperbyte	= larg->value;
		now			= WHICH(larg->value,
						DEFAULTLINK.costperbyte);
		break;
    case CPF:	larg->la->costperframe	= larg->value;
		now			= WHICH(larg->value,
						DEFAULTLINK.costperframe);
		break;
    case PFC:	larg->la->probframecorrupt= larg->value;
		now			= WHICH(larg->value,
						DEFAULTLINK.probframecorrupt);
		break;
    case PFL:	larg->la->probframeloss	= larg->value;
		now			= WHICH(larg->value,
						DEFAULTLINK.probframeloss);
		break;
    }
    if(vflag) {
	if(larg->from != DEFAULT)
	    sprintf(chararray,"(%s -> %s)",
			NP[larg->from].nodename,NP[larg->to].nodename);
	fprintf(stderr,"%s.%s = %d\n",
			(larg->from==DEFAULT) ? "defaultlink" : chararray,
			sliders[larg->n].keyword, now);
    }
}

/*  NEVER CALLED FOR AN_ETHERNET LINK */
static int link_scale(ClientData data,Tcl_Interp *interp,int argc,char *argv[])
{
    LINKARG	*larg;
    int		from, to, n;

    if(vflag) {
	for(n=0 ; n<argc-1 ; n++)
	    fprintf(stderr,"%s ", argv[n]);
	fprintf(stderr,"%s\n", argv[argc-1]);
    }
    if(argc != 5) {
	interp->result	= "wrong # args";
	return TCL_ERROR;
    }

    if(strcmp(argv[1], "default") == 0)
	from = to = DEFAULT;
    else {
	from	= atoi(argv[1]);
	to	= atoi(argv[2]);
    }
    larg	= make_linkarg(from, to);
    larg->n	= atoi(argv[3]);
    larg->value	= atoi(argv[4]);

    if(larg->n < 0 || larg->n > N_LINK_SLIDERS) {
	free(larg);
	interp->result	= "invalid link scale #";
	return TCL_ERROR;
    }

    link_slider_proc(larg);
    free(larg);
    return TCL_OK;
}

int link_created(ClientData data, Tcl_Interp *interp, int argc, char *argv[])
{
    LINKARG	*larg;
    static int	first_time	= TRUE;
    char	tmpbuf[32];
    int		from, to, n;

/*  Called as:	link_created $from $to */

    if(vflag) {
	for(n=0 ; n<argc-1 ; n++)
	    fprintf(stderr,"%s ", argv[n]);
	fprintf(stderr,"%s\n", argv[argc-1]);
    }
    if(argc != 3) {
	interp->result	= "wrong # args";
	return TCL_ERROR;
    }

    if(strcmp(argv[1], "eth") == 0) {
	n	= atoi(argv[2]);
	sprintf(chararray,"link_displayed(eth,%d)", n);
	Tcl_LinkVar(tcl_interp, chararray,
			(char *)&(ethernets[n].panel.displayed),
			TCL_LINK_BOOLEAN);
	return TCL_OK;
    }

    if(strcmp(argv[1], "default") == 0)
	from = to	= DEFAULT;
    else {
	from		= atoi(argv[1]);
	to		= atoi(argv[2]);
    }
    larg		= make_linkarg(from, to);
    sliders[CPB].value	= larg->la->costperbyte;
    sliders[CPF].value	= larg->la->costperframe;
    sliders[PFC].value	= larg->la->probframecorrupt;
    sliders[PFL].value	= larg->la->probframeloss;

    if(first_time) {	/* not done if we're AN_ETHERNET */
	TCLTK_createcommand("link_scale", link_scale);

	for(n=0 ; n<N_LINK_SLIDERS ; n++) {
	    sprintf(chararray,"link_scale_title(%d)",n);
	    Tcl_SetVar(tcl_interp, chararray,sliders[n].label,TCL_GLOBAL_ONLY);

	    sprintf(chararray,"link_scale_max(%d)",n);
	    sprintf(tmpbuf   ,"%d", sliders[n].max);
	    Tcl_SetVar(tcl_interp, chararray, tmpbuf, TCL_GLOBAL_ONLY);
	}
	first_time = FALSE;
    }

    for(n=0 ; n<N_LINK_SLIDERS ; n++) {
	sprintf(chararray,"link_scale_value(%d)", n);
	sprintf(tmpbuf, "%d", sliders[n].value);
	Tcl_SetVar(tcl_interp, chararray, tmpbuf, TCL_GLOBAL_ONLY);
    }

    if(from != DEFAULT) {
	sprintf(chararray, "link_displayed(%d,%d)", larg->from,larg->to);
	Tcl_LinkVar(tcl_interp, chararray,
			    (char *)&(larg->lp->displayed), TCL_LINK_BOOLEAN);
    }
    free(larg);
    return TCL_OK;
}

/* --------------------------------------------------------------------- */

void toggle_link_frame(LINKARG *larg, int x, int y)
{
    /* called from	main_windows.c:canvas_events() */

    if(larg->from >= AN_ETHERNET)
	TCLTK("ToggleLinkAttributes eth %d \"ethernet %s\" .%s %d %d",
			    larg->from - AN_ETHERNET,
			    ethernets[larg->from-AN_ETHERNET].name,
			    argv0,
			    x, y);
    else
	TCLTK("ToggleLinkAttributes %d %d \"%s->%s\" .%s %d %d",
			    larg->from, larg->to,
			    NP[larg->from].nodename, NP[larg->to].nodename,
			    argv0,
			    x, y);
}

#endif	/* defined(USE_TCLTK) */

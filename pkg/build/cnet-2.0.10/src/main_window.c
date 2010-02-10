#include "cnetheader.h"
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

/*  ------------ Nothing in here for USE_ASCII compilation ------------ */

#if	defined(USE_TCLTK)

void TCLTK(const char *fmt, ...)
{
#if	defined(__GNUC__)
    extern int	vsprintf(char *, const char *, va_list);
#endif

    va_list	ap;

    va_start(ap,fmt);
    vsprintf(chararray,fmt,ap);
    va_end(ap);

    if(vflag)
	fprintf(stderr,"%s\n", chararray);
    if(Tcl_Eval(tcl_interp, chararray) != TCL_OK) {
	if(*tcl_interp->result)
	    fprintf(stderr,"%s: %s\n", argv0, tcl_interp->result);
	cleanup(1);
    }
}


/* ------------------------------------------------------------------- */

static int exit_button(ClientData data,Tcl_Interp *interp,int argc,char *argv[])
{
    cnet_state  = STATE_UNKNOWN;
    cleanup(0);
    return TCL_OK;
}

static int run_button(ClientData data,Tcl_Interp *interp,int argc,char *argv[])
{
    if(strcmp(argv[1], "run") == 0) {
        if(cnet_state == STATE_PAUSED) {
	    Tcl_SetVar(tcl_interp, "CN_RUN_STRING", "Pause", 0);
            cnet_state  = STATE_RUNNING;
            tcltk_notify_stop();
        }
        else if(cnet_state == STATE_RUNNING) {
	    Tcl_SetVar(tcl_interp, "CN_RUN_STRING", "Run", 0);
            cnet_state  = STATE_PAUSED;
        }
    }
    else /* (argv[1] == "step") */ {
        cnet_state      = STATE_SINGLESTEP;
        tcltk_notify_stop();
    }
    return TCL_OK;
}

static int save_button(ClientData data,Tcl_Interp *interp,int argc,char *argv[])
{
    extern void	save_topology(char *);

    save_topology(argc > 1 ? argv[1] : (char *)NULL);
    return TCL_OK;
}


/* ------------------- Auxiliary main_window functions ---------------- */

/* These functions determine the (square of the) distance between a point
   and a line segment in 2 dimensions.
   All calculations are performed in integers (here pixels).

   As a side-effect, parameter t of point_to_line_segment_distance_2D()
   indicates the closest endpoint of the line segment to the given point.

		(x1,y1)  <  point <  (x2,y2)
		  1.0    <-   t   <-   0.0

   Thanks to Chris Pudney (chrisp@csse.uwa.edu.au) for his original
   floating point implementation which also found the closest point.
 */


static int euclid_distance_2D(int x1, int y1,int x2, int y2)
/* square of Euclidean distance */
{
    return ((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}


static int point_to_line_segment_distance_2D(int pointx,int pointy,
				int x1, int y1, int x2, int y2, float *t)
{
    int		len;

    len = euclid_distance_2D(x1,y1, x2,y2);

    /* Determine closest point along infinite line through the endpoints. */

    *t	= (float)((x1-x2)*(pointx-x2) + (y1-y2)*(pointy-y2)) / (float)len;

    /* Determine where along line equation closest point is. */

    if(*t >= 1.0)				/* then is beyond endpoint1 */
	return euclid_distance_2D(pointx,pointy, x1,y1);
    else if(*t <= 0.0)				/* then is beyond endpoint2 */
	return euclid_distance_2D(pointx,pointy, x2,y2);
    else {					/* else is between endpoints */
	int	tmpx = 	(int)((x1-x2)*(*t)) + x2,
		tmpy =	(int)((y1-y2)*(*t)) + y2;

	return euclid_distance_2D(pointx,pointy, tmpx,tmpy);
    }
}


/* ------------------ EVENT_HANDLE/REFRESH/REDRAW canvas------------------- */

void draw_link(int l, int clear_first, int width)
{
    LINK	*lp = &LP[l];

    if(clear_first)
	TCLTK(".%s.map delete l%d_%d", argv0, lp->endmin, lp->endmax);
    TCLTK(".%s.map create line %d %d %d %d -fill %s -width %d -tags {ls l%d_%d}",
		    argv0,
		    NP[lp->endmin].nattr.x, NP[lp->endmin].nattr.y,
		    NP[lp->endmax].nattr.x, NP[lp->endmax].nattr.y,
		    lp->linkup ? COLOUR_POINT2POINT : COLOUR_SEVERED,
		    width,
		    lp->endmin, lp->endmax);

    if(gattr.showcostperframe || gattr.showcostperbyte) {
#define	H2	6
#define	W2	3

	char	buf[64];
	int	width;
	int	midx	= NP[lp->endmin].nattr.x +
			    (NP[lp->endmax].nattr.x - NP[lp->endmin].nattr.x)/2;
	int	midy	= NP[lp->endmin].nattr.y +
			    (NP[lp->endmax].nattr.y - NP[lp->endmin].nattr.y)/2;
	int	cmin, cmax;

	if(gattr.showcostperframe)
	    cmin = WHICH(lp->lattrmin.costperframe, DEFAULTLINK.costperframe),
	    cmax = WHICH(lp->lattrmax.costperframe, DEFAULTLINK.costperframe);
	else
	    cmin = WHICH(lp->lattrmin.costperbyte , DEFAULTLINK.costperbyte ),
	    cmax = WHICH(lp->lattrmax.costperbyte , DEFAULTLINK.costperbyte );

	if(cmin == cmax)
	    sprintf(buf, "%d", cmin);
	else
	    sprintf(buf, "%d/%d", cmin, cmax);

	width	= strlen(buf)*W2 + 2;
	TCLTK(".%s.map create rectangle %d %d %d %d -fill %s -outline %s",
		    argv0, midx - width, midy - H2, midx + width, midy + H2,
		    "yellow", "black" );
	TCLTK(".%s.map create text %d %d -anchor c -font 5x7 -text \"%s\"",
		    argv0, midx, midy, buf);
#undef	W2
#undef	H2
    }
}

void draw_map(int width, int height)
{
    extern void	draw_ethernets(void);
    static int	done	= FALSE;

    CnetInt64	av_frames, tmp64;
    int		l, n, w;

/*  FIRSTLY, DRAW ANY BACKGROUND IMAGE OR LINES, ONCE */
    if(!done) {
	if(gattr.bgimage)
	    TCLTK(".%s.map create image %d %d -image im_bg -anchor c",
				    argv0, width/2, height/2);
	else {
	    char	cmdbuf[BUFSIZ], *p = cmdbuf;

#define	GRID_COLOUR	"#dddddd"
	    if(width < DRAWFRAME_WIDTH)
		width	= DRAWFRAME_WIDTH;
	    sprintf(p, "set x 20 ; set y 20\n");
	    while(*p)	++p;
	    sprintf(p,
    "while {$x < %d} { .%s.map create line $x 0 $x %d -fill %s ; incr x 20 }\n",
				    width, argv0, height, GRID_COLOUR);
	    while(*p)	++p;
	    sprintf(p,
    "while {$y < %d} { .%s.map create line 0 $y %d $y -fill %s ; incr y 20 }\n",
				    height, argv0, width, GRID_COLOUR);
	    TCLTK(cmdbuf);
#undef	GRID_COLOUR
	}
	done	= TRUE;
    }

/*  NEXT, REMOVE THE OLD LINKS */
    TCLTK(".%s.map delete ls", argv0);

/*  NEXT, CALCULATE THE AVERAGE TRAFFIC PER LINK */
    av_frames  = int64_ZERO;
    for(l=0 ; l<_NLINKS ; l++)
	if(LP[l].linkup)
	    int64_ADD(av_frames, av_frames, LP[l].ntxed);

    int64_I2L(tmp64, _NLINKS);
    int64_DIV(av_frames, av_frames, tmp64);

    draw_ethernets();

/*  THEN, DRAW ALL THE POINT2POINT LINKS */
    for(l=0 ; l<_NLINKS ; l++) {
	if(LP[l].linktype != LT_POINT2POINT)
	    continue;

	if(int64_CMP(LP[l].ntxed, <, av_frames))
	    w = 1;
	else {
	    int64_I2L(tmp64, 2);
	    int64_MUL(tmp64, tmp64, av_frames);
	    if(int64_CMP(LP[l].ntxed, <=, av_frames))
		w = CANVAS_FATLINK/2;
	    else
		w = CANVAS_FATLINK;
	}
	LP[l].ntxed	= int64_ZERO;
	draw_link(l, FALSE, w);
    }

/*  AND FINALLY, DRAW THE LINK NUMBERS AROUND EACH NODE AND THE NODE ICONS */
    for(n=0 ; n<_NNODES ; ++n)
	draw_node_icon(FALSE, n);
}

static int map_click(ClientData data,Tcl_Interp *interp, int argc, char *argv[])
{
    extern void	toggle_node_window(int);
    extern void	toggle_link_frame(LINKARG *, int, int);
    extern void	display_linkmenu(int, int, int);
    extern void	display_nodemenu(int);

    int		mindist, closest = 0;
    float	closest_endpoint = 0.0;
    int		n, evx, evy, but;

    if(vflag) {
	for(n=0 ; n<argc-1 ; ++n)
	    fprintf(stderr,"%s ", argv[n]);
	fprintf(stderr,"%s\n", argv[argc-1]);
    }
    if(argc != 4) {
	interp->result	= "wrong # args";
	return TCL_ERROR;
    }
    evx	= atoi(argv[1]);
    evy	= atoi(argv[2]);
    but	= atoi(argv[3]);

#define	CLOSE_ENOUGH	20

/*  WAS THE MOUSE EVENT CLOSE ENOUGH TO A NODE ICON? */
    for(n=0 ; n<_NNODES ; ++n) {
	int	x = NP[n].nattr.x;
	int	y = NP[n].nattr.y;

	if(evx > (x-CLOSE_ENOUGH) && evx < (x+CLOSE_ENOUGH) &&
	   evy > (y-CLOSE_ENOUGH) && evy < (y+CLOSE_ENOUGH))	{

	    if(but == 1) {
		toggle_node_window(n);
		return TCL_OK;
	    }
	    if(cnet_state == STATE_RUNNING)
		display_nodemenu(n);
	    return TCL_OK;
	}
    }

/*  WAS THE MOUSE EVENT CLOSE ENOUGH TO A LINK? */
    mindist	= CLOSE_ENOUGH;
    for(n=0 ; n<_NLINKS ; ++n) {		/* close enough to a link? */

	float	which_endpoint;
	int	dist;

	if(LP[n].linktype == LT_ETHERNET) {
	    ETHERNET	*ep	= &ethernets[LP[n].endmax];

	    dist = point_to_line_segment_distance_2D(
			evx,	evy,
			ep->x,	ep->y,
			ep->x + ep->nnics * DEF_NODE_X, ep->y,
			&which_endpoint);
	}
	else
	    dist = point_to_line_segment_distance_2D(
			evx,		evy,
			NP[LP[n].endmin].nattr.x, NP[LP[n].endmin].nattr.y,
			NP[LP[n].endmax].nattr.x, NP[LP[n].endmax].nattr.y,
			&which_endpoint);

	if(mindist > dist) {
	    mindist		= dist;
	    closest		= n;
	    closest_endpoint	= which_endpoint;
	}
    }
    if(mindist < CLOSE_ENOUGH) {
	LINK	*lp	= &LP[closest];

	if(but == 1) {
	    LINKARG	larg;

	    larg.link	= lp;
	    if(lp->linktype == LT_ETHERNET) {
		larg.lp		= &ethernets[lp->endmax].panel;
		larg.from	= AN_ETHERNET + lp->endmax;
		larg.to		= AN_ETHERNET + lp->endmax;
	    }
	    else if(closest_endpoint > 0.5) {
		larg.lp		= &lp->panelmin;
		larg.from	= lp->endmin;
		larg.to		= lp->endmax;
	    }
	    else {
		larg.lp		= &lp->panelmax;
		larg.from	= lp->endmax;
		larg.to		= lp->endmin;
	    }
	    toggle_link_frame(&larg, evx, evy);
	}
	else if(lp->linktype == LT_POINT2POINT &&
		(cnet_state == STATE_RUNNING || cnet_state == STATE_PAUSED))
	    display_linkmenu(closest, evx, evy);
    }
#undef	CLOSE_ENOUGH
    return TCL_OK;
}

void init_mainwindow(char *Fflag, int gflag, char *topology)
{
    extern void	find_mapsize(int *, int *);
    extern void	format_node_defaults(int);
    extern int	node_choice(ClientData, Tcl_Interp *, int, char **);
    extern int	link_created(ClientData, Tcl_Interp *, int, char **);

    char	*tcltk_source;
    int		width, height;

#if	defined(USE_BEFORE_TCL75)
    tcl_mainwindow = Tk_CreateMainWindow(tcl_interp, (char *)NULL,
					 argv0, argv0);
#else
    if(Tcl_Init(tcl_interp) != TCL_OK || Tk_Init(tcl_interp) != TCL_OK) {
	if(*tcl_interp->result)
	    fprintf(stderr,"%s: %s\n", argv0, tcl_interp->result);
	cleanup(1);
    }
    tcl_mainwindow = Tk_MainWindow(tcl_interp);
#endif

    if(tcl_mainwindow == NULL) {
        fprintf(stderr, "%s\n", tcl_interp->result);
        exit(1);
    }
    Tk_GeometryRequest(tcl_mainwindow, 200, 200);

#if	defined(USE_BEFORE_TCL75)
    if(Tcl_Init(tcl_interp) != TCL_OK || Tk_Init(tcl_interp) != TCL_OK) {
	if(*tcl_interp->result)
	    fprintf(stderr,"%s: %s\n", argv0, tcl_interp->result);
	cleanup(1);
    }
#endif

    tcltk_source = find_cnetfile(Fflag, FALSE, TRUE);
    if(dflag)
	fprintf(stderr,"reading \"%s\"\n\n", tcltk_source);
    if(Tcl_EvalFile(tcl_interp, tcltk_source) != TCL_OK) {
	if(*tcl_interp->result)
	    fprintf(stderr,"%s: %s\n", argv0, tcl_interp->result);
	cleanup(1);
    }
    free(tcltk_source);

    format_node_defaults(DEFAULT);

    Tcl_SetVar(tcl_interp, "CN_RUN_STRING", gflag ? "Pause" : "Run", 0);
    TCLTK("InitMainWindow %s %s %s %s %s",
	    argv0, topology, "exit_button", "run_step_button", "save_topology");

    find_mapsize(&width, &height);
    TCLTK("InitCanvas .%s.map %d %d", argv0, width, height);

    if(gattr.drawframes) {
	extern	void	init_drawframes(void);

	TCLTK("InitCanvas .%s.df %d %d", argv0, DRAWFRAME_WIDTH, 40);
	init_drawframes();
    }

    TCLTK_createcommand("exit_button",		exit_button);
    TCLTK_createcommand("link_created",		link_created);
    TCLTK_createcommand("map_click",		map_click);
    TCLTK_createcommand("node_choice",		node_choice);
    TCLTK_createcommand("run_step_button",	run_button);
    TCLTK_createcommand("save_topology",	save_button);

    draw_map(width, height);

#if	defined(CNET_EMAIL)
    if(Tk_GetFont(tcl_interp, tcl_mainwindow, "5x7") != (Tk_Font)NULL)
	TCLTK(".%s.map create text 0 %d -anchor sw -font 5x7 -text \" %s, %s\"",
			argv0, height+1, CNET_VERSION, CNET_EMAIL);
#endif
    TCLTK("bind .%s.map <Button> \"map_click %%x %%y %%b\"", argv0);
}

#endif		/* defined(USE_TCLTK) */

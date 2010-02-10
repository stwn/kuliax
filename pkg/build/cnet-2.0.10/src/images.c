#include  "cnetheader.h"
#include  <math.h>

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

/*  cnet is indebted to Brian Wellington <bwelling@tis.com> and Matias Duarte
    <matias@hyperimage.com>, the authors of XBill (v1.1 and beyond),
    for their pixmaps of various machines and operating system logos.

    I have modified and converted their pixmaps to GIFs, but the
    hard yards were certainly covered by the XBill authors.  XBill's
    official distribution site is    defenestration.dorm.umd.edu:/pub/xbill
 */

static char *os_names[] = {
  "bsd", "hurd", "linux", "mac", "next", "os2", "palm", "sgi", "sun", "windows"
};

#define	N_OS_NAMES	(sizeof(os_names) / sizeof(os_names[0]))

#if   !defined(USE_TCLTK)

char *random_osname(int randint)
{
    return(os_names[randint % N_OS_NAMES]);
}

#else

/* ----------------- only USE_TCLTK code follows ----------------------- */

char *random_osname(int randint)
{
    int		n;
    int		try = randint % N_OS_NAMES;
    char	*s;
    char	gif_name[32];

    for(n=0 ; n<N_OS_NAMES ; ++n) {
	sprintf(gif_name, "%s.gif", os_names[try]);
	s	= find_cnetfile(gif_name, FALSE, FALSE);
	if(s) {
	    free(s);
	    return(os_names[try]);
	}
	try = (try+1)%N_OS_NAMES;
    }
    fprintf(stderr,"%s: no operating system icons found via CNETPATH\n",argv0);
    cleanup(1);
    return((char *)NULL);
}

static	int	image_maxw	= 0;
static	int	image_maxh	= 0;

static void load_image(char *filenm)
{
    int		h, w;
    char	*path;
    char	gif_name[32];

    sprintf(gif_name, "%s.gif", filenm);
    path	= find_cnetfile(gif_name, FALSE, TRUE);
    TCLTK("image create photo im_%s -file %s", filenm, path);
    free(path);

    TCLTK("image height im_%s", filenm);
    h	= atoi(tcl_interp->result);
    if(image_maxh < h)
	image_maxh	= h;

    TCLTK("image width im_%s", filenm);
    w	= atoi(tcl_interp->result);
    if(image_maxw < w)
	image_maxw	= w;
}

static void init_images()
{
    char	**os_wanted;
    int		n_os_wanted;
    int		i,n;

    os_wanted	= (char **)malloc(_NNODES * sizeof(char *));
    memset(os_wanted, 0, _NNODES * sizeof(char *));
    n_os_wanted	= 0;

    for(n=0 ; n<_NNODES ; ++n)
	if(NP[n].nodetype == NT_HOST) {
	    for(i=0 ; i<n_os_wanted ; ++i)
		if(strcmp(os_wanted[i],NP[n].nattr.osname) == 0)
		    break;
	    if(i == n_os_wanted) {
		load_image(NP[n].nattr.osname);
		os_wanted[n_os_wanted++] = NP[n].nattr.osname;
	    }
	}
    free(os_wanted);

    for(n=0 ; n<_NNODES ; ++n)
	if(NP[n].nodetype == NT_ROUTER) {
	    load_image("router");
	    break;
	}
    load_image("dead");
    load_image("paused");
    load_image("repair");

    if(gattr.drawframes)
	load_image("zap");
}

void find_mapsize(int *width, int *height)
{
    NODEATTR	*na;
    int		n;
    int		minx	= int32_MAXINT;
    int		miny	= int32_MAXINT;
    int		maxx	= 0;
    int		maxy	= 0;

    init_images();

/*  IF WE HAVE A BACKGROUND IMAGE, DETERMINE (SOME) DIMENSIONS FROM IT */
    if(gattr.bgimage) {
	int	h, w;
	char	*s;

	s	= find_cnetfile(gattr.bgimage, FALSE, TRUE);
	TCLTK("image create photo im_bg -file %s", s);
	TCLTK("image height im_bg");
	h	= atoi(tcl_interp->result);
	if(maxy < h)
	    maxy	= h;

	TCLTK("image width im_bg");
	w	= atoi(tcl_interp->result);
	if(maxx < w)
	    maxx	= w;
    }

/*  SEE IF ANY OF THE NODES' x AND y CO-ORDINATES WOULD PRINT OFF THE MAP */
    for(n=0 ; n<_NNODES ; ++n) {
	na      = &NP[n].nattr;
	if(minx > na->x)
	    minx = na->x;
	if(miny > na->y)
	    miny = na->y;
    }

/*  IF WOULD PRINT OFF THE MAP, SHIFT WHOLE MAP DOWN AND RIGHT */
    if(minx < image_maxw)
	for(n=0 ; n<_NNODES ; n++)
	    NP[n].nattr.x -= (minx - image_maxw);
    if(miny < image_maxh)
	for(n=0 ; n<_NNODES ; n++)
	    NP[n].nattr.y -= (miny - image_maxh);

/*  NOW, DETERMINE THE MAXIMUM DIMENSTIONS OF THE WHOLE MAP */
    for(n=0 ; n<_NNODES ; ++n) {
	na      = &NP[n].nattr;
	if(na->x > maxx)
	    maxx = na->x;
	if(na->y > maxy)
	    maxy = na->y;
    }

    *width  = maxx + image_maxw;
    *height = maxy + image_maxh;
}

void draw_node_icon(int just_clear, int n)
{
    NODE	*np;
    int		dx, dy, x, y;
    int		l, w, otherend;
    double	theta;
    char	*name = "";

    if(just_clear) {
	TCLTK(".%s.map delete n%d", argv0, n);
	return;
    }

    np	= &NP[n];
    switch ((int)(np->runstate)) {
    case STATE_RUNNING :
    case STATE_REBOOTING :
    case STATE_AUTOREBOOT :
	TCLTK(".%s.map create image %d %d -image im_%s -anchor c -tags n%d",
		argv0, np->nattr.x, np->nattr.y,
		name = (np->nodetype == NT_HOST ? np->nattr.osname : "router"),
		n);
	break;

    case STATE_UNDERREPAIR :
	TCLTK(".%s.map create image %d %d -image im_%s -anchor c -tags n%d",
		argv0, np->nattr.x, np->nattr.y, name="repair", n);
	break;

    case STATE_CRASHED :
	TCLTK(".%s.map delete n%d", argv0, n);
	TCLTK(".%s.map create image %d %d -image im_%s -anchor c -tags n%d",
		argv0, np->nattr.x, np->nattr.y, name="dead", n);
	break;

    case STATE_PAUSED :
	TCLTK(".%s.map create image %d %d -image im_%s -anchor c -tags n%d",
		argv0, np->nattr.x, np->nattr.y, name="paused", n);
	break;
    }

    TCLTK("global stdiofont labelfont");
    for(l=1 ; l<=np->nlinks ; l++) {
	w = np->links[l];
	if(LP[w].linktype != LT_POINT2POINT)
	    continue;

	otherend = (LP[w].endmin == n) ? LP[w].endmax : LP[w].endmin;

	dx	= NP[otherend].nattr.x - np->nattr.x;
	dy	= NP[otherend].nattr.y - np->nattr.y;
	theta	= atan2((double)dy, (double)dx) + 0.3;
	x	= np->nattr.x + (int)(image_maxw/1.5*cos(theta));
	y	= np->nattr.y + (int)(image_maxh/1.5*sin(theta));

	TCLTK(
    ".%s.map create text %d %d -anchor c -font $stdiofont -tags n%d -text %d",
		argv0, x, y, n, l);
    }
    TCLTK("image height im_%s", name);
    l	= atoi(tcl_interp->result);
    TCLTK(
".%s.map create text %d %d -anchor n -font $labelfont -tags n%d -text \"%d: %s\"",
		argv0, np->nattr.x, np->nattr.y+l/2+2, n, n, np->nodename);
}

#endif		/* defined(USE_TCLTK) */

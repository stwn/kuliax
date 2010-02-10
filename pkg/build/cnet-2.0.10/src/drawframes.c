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

/*  ------------ Nothing in here for USE_ASCII compilation ------------ */

#if	defined(USE_TCLTK)

#define	TIME_SHOW_ZAP	400000

#define	N_DFS		16
static	DRAWFRAME	*dfs[N_DFS]	= {0};
static	int		ndfs		= 0;

void new_drawframe(int len, char *frame, CnetInt64 delay, CnetInt64 Tprop,
		   int is_lost, int is_corrupt)
{
    DRAWFRAME	*df;
    CnetInt64	two, Ton2;

    df		= NEW(DRAWFRAME);
    memset(df, 0, sizeof(DRAWFRAME));

    int64_ADD(df->started, TIMENOW_in_USEC, delay);
    int64_ADD(df->arrives, df->started, Tprop);
    df->inflight	= Tprop;

    int64_I2L(two, 2);
    int64_DIV(Ton2, Tprop, two);
    if(is_lost) {
	int64_ADD(df->when_lost, df->started, Ton2);
    }
    else
	df->when_lost	= int64_MAXINT;
    if(is_corrupt) {
	int64_ADD(df->when_corrupt, df->started, Ton2);
    }
    else
	df->when_corrupt = int64_MAXINT;

    df->cdf.text[0]	= '\0';
    df->cdf.len		= len;
    df->cdf.frame	= malloc(len);
    memcpy(df->cdf.frame, frame, len);

/*  USER-CODE WILL ADD THE COLOURS, PIXEL LENGTHS, AND STRING REQUIRED */
    schedule_event(EV_DRAWFRAME, THISNODE, delay, NULLTIMER, (CnetData)df);
}

void free_drawframe(DRAWFRAME *df)
{
    free(df->cdf.frame);
    free(df);
}

/*  CALLED AFTER USER-CODE HAS ADDED COLOURS, PIXEL LENGTHS, AND STRING */
void add_drawframe(DRAWFRAME *df)
{
    char	buf[BUFSIZ], *s, *t;
    int		n, nf;

    free(df->cdf.frame);
    df->cdf.len	= 0;

/*  ENSURE THAT AT LEAST ONE VALID COLOUR AND LENGTH WAS PROVIDED */
    for(nf=0 ; nf<CN_NCOLOURS ; ++nf)
	if(df->cdf.colour[nf] <= 0 || df->cdf.pixels[nf] <= 0)
	    break;
    if(nf == 0) {
	free(df);
	return;
    }

/*  ENSURE THAT WE HAVE A FREE SLOT TO REMEMBER THIS DRAWFRAME */
    for(n=0 ; n<N_DFS ; ++n)
	if(dfs[n] == (DRAWFRAME *)NULL)
	    break;
    if(n == N_DFS) {
	free(df);
	return;
    }

    df->cdf.text[DRAWFRAME_TEXTLEN-1]	= '\0';
    dfs[n]	= df;
    ++ndfs;

    df->srcnode	= THISNODE;

/*  BUILD THE TCL/TK COMMAND IN buf */
    s	= buf;
    sprintf(buf, "DrawFrame %s %d %d %d df%d ",
		argv0, nf, NP[THISNODE].nattr.x, THISNODE == 0 ? 6 : 25, n);
    while(*s) ++s;
    *s++	= '{';
    for(n=0 ; n<nf ; ++n) {
	sprintf(s, "%d ", ((df->cdf.colour[n]-1) % CN_NCOLOURS)+1);
	while(*s) ++s;
    }
    *s++	= '}';
    *s++	= ' ';
    *s++	= '{';
    for(n=0 ; n<nf ; ++n) {
	sprintf(s, "%d ", df->cdf.pixels[n] % 101);
	while(*s) ++s;
    }
    *s++	= '}';
    sprintf(s, " %d ", THISNODE == 0 ? -1 : 1);

    while(*s) ++s;
    *s++	= '"';
    t		= df->cdf.text;
    while(*t) {
	if(*t == '"')
	    *s++ = '\\';
	*s++	 = *t++;
    }
    *s++	= '"';
    *s++	= '\0';
    TCLTK(buf);
}

void lose_all_drawframes(void)
{
    int	n;

    for(n=0 ; n<N_DFS ; ++n)
	if(dfs[n]) {
	    TCLTK(".%s.df itemconfigure df%d -fill white", argv0, n);
	    dfs[n]->when_lost	= int64_MAXINT;
	}
}


/*  RE-COLOUR, DELETE, AND MOVE ALL OF THE FRAMES CURRENTLY IN TRANSIT */

static CnetInt64	zap_time;
static CnetInt64	xwidth;

void init_drawframes()
{
    zap_time	= int64_MAXINT;
    int64_I2L(xwidth, NP[1].nattr.x - NP[0].nattr.x);
}

void move_drawframes()
{
    DRAWFRAME	*df;
    int		n, v;

/*  ANYTHING TO DRAW? */
    if(ndfs == 0)
	return;

    v		= vflag;
    vflag	= FALSE;
    for(n=0 ; n<N_DFS ; ++n) {
	if(dfs[n] == (DRAWFRAME *)NULL)
	    continue;
	df	= dfs[n];

/*  STILL MOVING */
	if(int64_CMP(TIMENOW_in_USEC, <, df->arrives)) {

	    CnetInt64	tmp64;
	    int		dx;

/*  IF FRAME GETS LOST, CHANGE ITS COLOUR TO WHITE */
	    if(int64_CMP(df->when_lost, <=, TIMENOW_in_USEC)) {
		TCLTK(".%s.df itemconfigure df%d -fill white", argv0, n);
		df->when_lost		= int64_MAXINT;
	    }
/*  IF FRAME GETS CORRUPTED, ZAP IT, AND CHANGE ITS COLOUR TO DARK-GREY */
	    else if(int64_CMP(df->when_corrupt, <=, TIMENOW_in_USEC)) {
		TCLTK(
		".%s.df create image %d %d -image im_zap -anchor w -tags dfzap",
			    argv0, NP[1].nattr.x / 2, 22);
		int64_I2L(zap_time, TIME_SHOW_ZAP);
		int64_ADD(zap_time, TIMENOW_in_USEC, zap_time);

		TCLTK(".%s.df itemconfigure df%d -fill grey50", argv0, n);
		df->when_corrupt	= int64_MAXINT;
	    }
	    else if(int64_CMP(zap_time, <=, TIMENOW_in_USEC)) {
		TCLTK(".%s.df delete dfzap", argv0);
		zap_time		= int64_MAXINT;
	    }

	    int64_SUB(tmp64, TIMENOW_in_USEC, df->started);
	    int64_MUL(tmp64, tmp64, xwidth);
	    int64_DIV(tmp64, tmp64, df->inflight);
	    int64_L2I(dx, tmp64);
	    dx		-= df->x;
	    df->x	+= dx;
	    TCLTK(".%s.df move df%d %d 0 ; .%s.df move tdf%d %d 0",
			    argv0,n,(df->srcnode==0) ? dx : -dx,
			    argv0,n,(df->srcnode==0) ? dx : -dx);
	}
/*  REACHED THE END OF ITS TRAVEL */
	else {
	    TCLTK(".%s.df delete df%d ; .%s.df delete tdf%d",
			    argv0, n, argv0, n);
	    free(df);
	    dfs[n]	= (DRAWFRAME *)NULL;
	    --ndfs;
	}
    }
    vflag	= v;
}

#endif		/* defined(USE_TCLTK) */

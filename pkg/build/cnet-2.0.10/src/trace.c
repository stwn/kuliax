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

typedef struct _n {
    struct _n	*next;
    void	*addr;
    char	*name;
} T_NAME;

static T_NAME	*t_name	= 	(T_NAME *)NULL;


/* --------------------------------------------------------------------- */

void clear_trace_names()
{
    T_NAME	*t;

    while(t_name) {
	t	= t_name;
	t_name	= t_name->next;
	free(t->name);
	free(t);
    }
    t_name	= (T_NAME *)NULL;
}

static char *annotate(const char *name)
{
    char *s;

    if(*name == '&')
	s	= strdup(name);
    else {
	s	= (char *)malloc(strlen(name)+2);
	s[0]	= '&';
	strcpy(&s[1], name);
    }
    return(s);
}

int CNET_trace_name(void *addr, const char *name)
{
    T_NAME	*t;

    if(addr == (void *)NULL || name == (char *)NULL || *name == '\0') {
	if(gattr.trace_events)
	    TRACE("\tCNET_trace_name(addr=%ld,name=%ld) = -1 ER_BADARG\n",
				    (long)addr, (long)name);
	cnet_errno = ER_BADARG;
	return(-1);
    }

#if 0
    if(gattr.trace_events)
	TRACE("\tCNET_trace_name(addr=0x%lx,name=\"%s\") = 0\n",
				(long)addr, name);
#endif
    t	= t_name;
    while(t) {
	if(t->addr == addr) {
	    free(t->name);
	    t->name	= annotate(name);
	    return(0);
	}
	t	= t->next;
    }
    t		= NEW(T_NAME);
    t->addr	= addr;
    t->name	= annotate(name);
    t->next	= t_name;
    t_name	= t;
    return 0;
}

char *find_trace_name(void *addr)
{
    T_NAME	*t;
    char	buf[32];

    if((void *)NULL == addr)
	return("NULL");

    t	= t_name;
    while(t) {
	if(t->addr == addr)
	    return(t->name);
	t	= t->next;
    }
    t		= NEW(T_NAME);
    t->addr	= addr;
    sprintf(buf, "0x%lx", (long)addr);
    t->name	= strdup(buf);
    t->next	= t_name;
    t_name	= t;
    return(t->name);
}

/* --------------------------------------------------------------------- */


void TRACE(const char *fmt, ...)
{
#if	defined(__GNUC__)
    extern int	vsprintf(char *, const char *, va_list);
#endif

    va_list	ap;
    char	buf[BUFSIZ];

#if	defined(USE_TCLTK)
    static int	ready = FALSE;
    char	tcltk_buf[BUFSIZ];
    char        *b, *t;
#endif

    va_start(ap,fmt);
    vsprintf(buf,fmt,ap);
    va_end(ap);

    if(gattr.tfp)			/* possibly mirror in trace-file */
	fputs(buf, gattr.tfp);

    if(!Wflag) {			/* if not under Tcl/Tk, just stderr */
	fputs(buf, stderr);
	return;
    }

#if	defined(USE_TCLTK)
    if(!ready) {
	TCLTK("InitTraceWindow .%s.tr", argv0);
	ready	= TRUE;
    }

    b		= buf;
    while(*b) {
	t	= tcltk_buf;
	while(*b && *b != '\n') {
	    if(*b == '"' || *b == '[' || *b == '\\')
		*t++ = '\\';		/* elide significant Tcl chars */
	    *t++ = *b++;
	}
	*t	= '\0';
	TCLTK("traceaddstr \"%s\" %d", tcltk_buf, (*b=='\n'));
	if(*b == '\n')
	    ++b;
    }
#endif
}

/* --------------------------------------------------------------------- */


int CNET_trace(const char *fmt, ...)
{
#if	defined(__GNUC__)
    extern int	vsprintf(char *, const char *, va_list);
#endif

    va_list	ap;

    if(gattr.trace_events) {
	char	buf[BUFSIZ];

	va_start(ap,fmt);
	vsprintf(buf,fmt,ap);
	va_end(ap);
	TRACE("\t%s", buf);
    }
    return 0;
}

static void print_tmask(int mask)
{
    int	e;

    if(mask == TE_NOEVENTS) {
	TRACE("TE_NOEVENTS");
	return;
    }
    for(e=0 ; e<N_CNET_EVENTS ; ++e) {
	if(mask & (1<<e)) {
	    mask = mask & ~(1<<e);
	    TRACE("TE_%s%s",	e==0 ? "ALLEVENTS" : &cnet_evname[e][3],
				mask ? "|" : "");
	}
    }
}

int CNET_get_trace()
{
    int mask	= NP[THISNODE].nattr.trace_mask ;

    if(gattr.trace_events) {
	TRACE("\tCNET_get_trace() = ");
	print_tmask(mask);
	TRACE("\n");
    }
    return(mask);
}

int CNET_set_trace(int newmask)
{
    int oldmask	= NP[THISNODE].nattr.trace_mask ;

    newmask = newmask % (1<<N_CNET_EVENTS);
    if(!gattr.trace_events && newmask)
	TRACE("nowin %s.%s @%susec\n", NP[THISNODE].nodename,
		    cnet_evname[(int)HANDLING], int64_L2A(TIMENOW_in_USEC,1));

    if(gattr.trace_events || newmask) {
	TRACE("\tCNET_set_trace(");
	print_tmask(newmask);
	TRACE(") = ");
	print_tmask(oldmask);
	TRACE("\n");
    }
    NP[THISNODE].nattr.trace_mask = newmask;
    gattr.trace_events =	DEFAULTNODE.trace_all		|
				NP[THISNODE].nattr.trace_all	|
				(newmask & (1<<(int)HANDLING));
    return(oldmask);
}

/* --------------------------------------------------------------------- */

void init_trace()
{
    if(gattr.trace_filenm) {
	if((gattr.tfp = fopen(gattr.trace_filenm, "w")) == (FILE *)NULL) {
	    fprintf(stderr,"%s: cannot create trace-file '%s'\n",
				argv0, gattr.trace_filenm);
	    cleanup(1);
	}
    }
}

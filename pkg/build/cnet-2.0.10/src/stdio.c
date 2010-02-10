#include <stdarg.h>
#include "cnetheader.h"
#include <fcntl.h>

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


#if	defined(USE_TCLTK)
static int tk_stdio_input(ClientData data, Tcl_Interp *interp,
				int argc, char *argv[])
{
    NODE	*np;
    int		n;

    if(vflag) {
	for(n=0 ; n<argc-1 ; ++n)
	    fprintf(stderr,"%s ", argv[n]);
	fprintf(stderr,"%s\n", argv[argc-1]);
    }
    if(argc != 3) {
	interp->result	= "wrong # args";
	return TCL_ERROR;
    }
    n		= atoi(argv[1]);
    if(n < -1 || n >= _NNODES) {
	interp->result	= "invalid node";
	return TCL_ERROR;
    }

    np	= &NP[n];
    if(np->handler[(int)EV_KEYBOARDREADY] && np->inputline == (char *)NULL) {
	np->inputline	= strdup(argv[2]);
	np->inputlen	= strlen(argv[2]);
	schedule_event(EV_KEYBOARDREADY, n, int64_ONE,
			NULLTIMER, np->data[(int)EV_KEYBOARDREADY]);
    }
    return TCL_OK;
}

void tk_init_stdio_window(int n, char *tk_name)
{
    Tk_Window	tkwin	= Tk_NameToWindow(tcl_interp, tk_name, tcl_mainwindow);

    Tk_MakeWindowExist(tkwin);
}
#endif	/* defined(USE_TCLTK) */


/* ------------------------------------------------------------------------- */

static void stdio_flush(char *str)
{
    NODE	*np;
    char	*t	= str;
    int		len	= 0;

#if	defined(USE_TCLTK)
    char	tcltk_buf[BUFSIZ];
#endif	/* defined(USE_TCLTK) */

    while(*t++)
	len++;

    np	= &(NP[THISNODE]);
    if(np->outputfd >= 0)		/* duplicate in local file */
	write(np->outputfd, str, len);

    if(gattr.stdio_quiet)
	return;

#if	defined(USE_TCLTK)
    while(*str) {
	t	= tcltk_buf;
	while(*str && *str != '\n') {
	    if(*str == '"' || *str == '[' || *str == '\\')
		*t++ = '\\';		/* elide significant Tcl chars */
	    *t++ = *str++;
	}
	*t	= '\0';
	TCLTK("stdioaddstr %d \"%s\" %d", THISNODE, tcltk_buf, (*str=='\n'));
	if(*str == '\n')
	    ++str;
    }
#endif	/* defined(USE_TCLTK) */
}

int printf(const char *fmt, ...)
{
#if	defined(__GNUC__)
    extern int	vsprintf(char *, const char *, va_list);
#endif

    va_list	ap;
    char	stdio_buf[BUFSIZ];

    if(gattr.stdio_quiet && NP[THISNODE].outputfd < 0)	/* faster! */
	return(0);

    va_start(ap,fmt);
    vsprintf(stdio_buf,fmt,ap);
    va_end(ap);
    stdio_flush(stdio_buf);
    return(0);
}

int puts(const char *str)		/* the substitute for puts() */
{
    char	stdio_buf[BUFSIZ];
    char	*p = stdio_buf;

    while((*p++ = *str++));
    *(p-1)	= '\n';
    *p		= '\0';
    stdio_flush(stdio_buf);
    return(0);
}

#if	defined(putchar)
#undef	putchar
#endif

int putchar(int ch)			/* the substitute for putchar() */
{
    char	stdio_buf[4];

    stdio_buf[0] = ch; stdio_buf[1] = '\0';
    stdio_flush(stdio_buf);
    return(0);
}


int CNET_read_keyboard(char *line, int *len)
{
    NODE	*np;
    int		result	= (-1);

    if(gattr.trace_events) {
	if(VALID_INTPTR(len) || *len <= 0)
	    TRACE("\tCNET_read_keyboard(%s,*len=%d) = ",
				find_trace_name(line), *len);
	else
	    TRACE("\tCNET_read_keyboard(%s,%s) = ",
				find_trace_name(line), find_trace_name(len));
    }
    np	 = &(NP[THISNODE]);
    if(np->nodetype != NT_HOST || !Wflag)
	cnet_errno		= ER_NOTSUPPORTED;

#if	defined(USE_TCLTK)
    else if(np->inputline == (char *)NULL)
	cnet_errno	= ER_NOTREADY;

    else if(*len <= np->inputlen)
	cnet_errno	= ER_BADSIZE;

    else {				/* only provide inputline once */
	strcpy(line, np->inputline);
	*len		= np->inputlen+1;	/* strlen() + NULL */
	free(np->inputline);
	np->inputline	= (char *)NULL;
	np->inputlen	= 0;
	result		= 0;
    }
#endif

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE("0 (len=%d)\n", *len);
	else
	    TRACE("-1 %s\n",cnet_errname[(int)cnet_errno]);
    }
    if(result != 0) {
	*line	= '\0';
	*len	= UNKNOWN;
    }
    return result;
}

void CNET_clear()
{
#if	defined(USE_TCLTK)
    if(Wflag)
	TCLTK("stdioclr %d", THISNODE);
#endif
}


/* -------------------- NO USER-SERVICEABLE CODE BELOW -------------------- */

void init_stdio_layer(char *oflag)
{
    NODE	*np;
    int		n;
    char	*outputfile;

    if(oflag)
	DEFAULTNODE.outputfile	= oflag;

    for(n=0 ; n<_NNODES ; n++) {
	np		= &(NP[n]);
	outputfile	= (char *)NULL;
	np->outputfd	= -1;
	if(DEFAULTNODE.outputfile) {
	    sprintf(chararray,"%s.%s", DEFAULTNODE.outputfile, np->nodename);
	    outputfile	= chararray;
	}
	if(np->nattr.outputfile)
	    outputfile	= np->nattr.outputfile;
	if(outputfile) {
	    if((np->outputfd =
		open(outputfile,O_WRONLY|O_CREAT|O_TRUNC,0600)) < 0) {
		fprintf(stderr,"%s: cannot create %s\n",argv0,outputfile);
		exit(2);
	    }
	}
    }

#if	defined(USE_TCLTK)
    if(Wflag)
	TCLTK_createcommand("stdio_input", tk_stdio_input);
#endif	/* defined(USE_TCLTK) */
}

void reboot_stdio_layer()
{
#if	defined(USE_TCLTK)
    if(Wflag) {
	NODE *np	= &NP[THISNODE];

	if(np->inputline)
	    free(np->inputline);
	np->inputline	= (char *)NULL;
	np->inputlen	= 0;
    }
#endif	/* defined(USE_TCLTK) */
}


/* --------------- ALL THE DEPRECATED FUNCTIONS, AS OF v1.5-1 ----------- */

int CNET_set_cursor(int row, int col)
{
    fprintf(stderr,"%s: CNET_set_cursor() is no longer supported\n", argv0);
    return(0);
}

int CNET_get_cursor(int *row, int *col)
{
    fprintf(stderr,"%s: CNET_get_cursor() is no longer supported\n", argv0);
    return(0);
}

void CNET_clear_to_eos()
{
    fprintf(stderr,"%s: CNET_clear_to_eos() is no longer supported\n", argv0);
}

void CNET_clear_to_eoln()
{
    fprintf(stderr,"%s: CNET_clear_to_eoln() is no longer supported\n", argv0);
}

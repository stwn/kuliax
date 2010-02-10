#include "cnetheader.h"

#if !defined(USE_WIN32)
#include <dlfcn.h>
#endif

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


/* ---------------- Application Layer Interface ------------------------- */

extern	int
	std_init_application_layer(int),
	std_application_bounds(int *minmsg, int *maxmsg),
	std_reboot_application_layer(CnetInt64 *),
	std_poll_application(CnetInt64 *),
	std_CNET_read_application(CnetAddr *,char *, int *),
	std_CNET_write_application(char *, int *),
	std_CNET_enable_application(CnetAddr),
	std_CNET_disable_application(CnetAddr);

enum { IAL=0, AB, RAL, PA, RA, WA, EA, DA };

static struct {
    char	*fn_name;
    int		(*fn)();
    char	*fn_proto;
} FUNCS[] = {
    { "init_application_layer",	std_init_application_layer,
	"void init_application_layer(int Sflag)"			},
    { "application_bounds",	std_application_bounds,
	"void application_bounds(int *minmsg, int *maxmsg)"		},
    { "reboot_application_layer",	std_reboot_application_layer,
	"int reboot_application_layer(CnetInt64 *ask_next)"		},
    { "poll_application",		std_poll_application,
	"int poll_application(CnetInt64 *ask_next)"			},
    { "CNET_read_application",		std_CNET_read_application,
	"int CNET_read_application(CnetAddr *destaddr, char *msg, int *len)" },
    { "CNET_write_application",		std_CNET_write_application,
	"int CNET_write_application(char *msg, int *len)"		},
    { "CNET_enable_application",	std_CNET_enable_application,
	"int CNET_enable_application(CnetAddr destaddr)"		},
    { "CNET_disable_application",	std_CNET_disable_application,
	"int CNET_disable_application(CnetAddr destaddr)"		},
};

#define	NFNS		(sizeof(FUNCS) / sizeof(FUNCS[0]))


void init_application_layer(char *Aflag, int kflag, int Sflag)
{
    if(Aflag) {
	extern char	*compile_string(char *, int);
	char		*so_file;

#if !defined(USE_WIN32)
	void		*handle;
	int		f;
#endif

	if((so_file = compile_string(Aflag, kflag)) == (char *)NULL)
	    cleanup(1);

#if !defined(USE_WIN32)
	if((handle = dlopen(so_file, RTLD_LAZY)) == NULL) {
	    fprintf(stderr,"%s: cannot load application layer from %s\n",
					argv0, so_file);
	    exit(1);
	}
	for(f=0 ; f<NFNS ; f++)
	    if((FUNCS[f].fn = (int (*)())dlsym(handle,FUNCS[f].fn_name)) ==
				NULL) {
		fprintf(stderr,"%s: cannot find '%s' in %s\n",
					argv0, FUNCS[f].fn_proto, so_file);
		++nerrors;
	    }
#endif
	if(nerrors) exit(1);

	/* we do *not* dlclose(handle) */
    }
    (void)(FUNCS[(int)IAL].fn)(Sflag);
}


/* ---------------------------------------------------------------------- */


int application_bounds(int *minmsg, int *maxmsg)
{					/* called iff THISNODE is a NT_HOST */
    return( (FUNCS[(int)AB].fn)(minmsg, maxmsg) );
}

int reboot_application_layer(CnetInt64 *ask_next)
{					/* called iff THISNODE is a NT_HOST */
    return( (FUNCS[(int)RAL].fn)(ask_next) );
}

int poll_application(CnetInt64 *ask_next)
{					/* called iff THISNODE is a NT_HOST */
    return( (FUNCS[(int)PA].fn)(ask_next) );
}


int CNET_read_application(CnetAddr *destaddr, char *msg, int *len)
{
    int	result;

    if(gattr.trace_events) {
	if(VALID_INTPTR(len))
	    TRACE("\tCNET_read_application(%s,%s,*len=%d) = ",
		    find_trace_name(destaddr), find_trace_name(msg), *len);
	else
	   TRACE("\tCNET_read_application(%s,%s,%s) = ",
	find_trace_name(destaddr), find_trace_name(msg), find_trace_name(len));
    }
    if(NP[THISNODE].nodetype != NT_HOST) {
	cnet_errno	= ER_NOTSUPPORTED;
	result		= -1;
    }
    else
	result		= (FUNCS[(int)RA].fn)(destaddr, msg, len);

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE("0 (*dest=%lu,*len=%d)\n", *destaddr, *len);
	else
	    TRACE("-1 %s\n",cnet_errname[(int)cnet_errno]);
    }
    return result;
}


int CNET_write_application(char *msg, int *len)
{
    int		result;

    if(gattr.trace_events) {
	if(VALID_INTPTR(len))
	    TRACE("\tCNET_write_application(%s,*len=%d) = ",
				find_trace_name(msg), *len);
	else
	    TRACE("\tCNET_write_application(%s,%s) = ",
				find_trace_name(msg), find_trace_name(len));
    }
    if(NP[THISNODE].nodetype != NT_HOST) {
	cnet_errno	= ER_NOTSUPPORTED;
	result		= -1;
    }
    else
	result		= (FUNCS[(int)WA].fn)(msg, len);

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE("0 (*len=%d)\n", *len);
	else
	    TRACE("-1 %s\n",cnet_errname[(int)cnet_errno]);
    }
    return result;
}


/* --------------    Enable/Disable Application Layer ------------------- */


int CNET_enable_application(CnetAddr destaddr)
{
    int	result;

    if(gattr.trace_events) {
	TRACE("\tCNET_enable_application");
	if(destaddr == ALLNODES)
	    TRACE("(ALLNODES) = ");
	else
	    TRACE("(dest=%lu) = ",destaddr);
    }

    if(NP[THISNODE].nodetype != NT_HOST) {
	cnet_errno	= ER_NOTSUPPORTED;
	result		= -1;
    }
    else
	result		= (FUNCS[(int)EA].fn)(destaddr);

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE("0\n",stderr);
	else
	    TRACE("-1 %s\n",cnet_errname[(int)cnet_errno]);
    }
    return result;
}

int CNET_disable_application(CnetAddr destaddr)
{
    int	result;

    if(gattr.trace_events) {
	TRACE("\tCNET_disable_application");
	if(destaddr == ALLNODES)
	    TRACE("(ALLNODES) = ");
	else
	    TRACE("(dest=%lu) = ",destaddr);
    }

    if(NP[THISNODE].nodetype != NT_HOST) {
	cnet_errno	= ER_NOTSUPPORTED;
	result		= -1;
    }
    else
	result		= (FUNCS[(int)DA].fn)(destaddr);

    if(gattr.trace_events) {
	if(result == 0)
	    TRACE("0\n",stderr);
	else
	    TRACE("-1 %s\n",cnet_errname[(int)cnet_errno]);
    }
    return result;
}

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


/* ------------------- Physical Layer Interface ------------------------- */

extern int		std_init_physical_layer(int, int, int, int);
extern int		std_reboot_physical_layer(int);
extern int		std_clear_physical_link(int);
extern int		std_poll_physical(CnetInt64 *when, int *);

extern int		std_CNET_read_physical(int *, char *, int *);
extern int		std_CNET_write_physical(int, char *, int *);
extern int		std_CNET_write_physical_reliable(int, char *, int *);
extern int		std_CNET_write_direct(CnetAddr,char *,int *);

enum { IPL=0, RPL, CPL, PP, RP, WP, WPR, WD };

static struct {
    char	*fn_name;
    int		(*fn)();
    char	*fn_proto;
} FUNCS[] = {
    { "init_physical_layer",		std_init_physical_layer	,
	"void init_physical_layer(int eflag, int Nflag, int Sflag)"	},
    { "reboot_physical_layer",		std_reboot_physical_layer,
	"int reboot_physical_layer(int node)"				},
    { "clear_physical_link",		std_clear_physical_link	,
	"int clear_physical_link(int linkno)"				},
    { "poll_physical",			std_poll_physical,
	"int poll_physical(CnetInt64 *when, int *node)"			},
    { "CNET_read_physical",		std_CNET_read_physical,
	"int CNET_read_physical(int *link, char *frame, int *len)"	},
    { "CNET_write_physical",		std_CNET_write_physical,
	"int CNET_write_physical(int link, char *frame, int *len)"	},
    { "CNET_write_physical_reliable",	std_CNET_write_physical_reliable,
	"int CNET_write_physical_reliable(int link, char *frame, int *len)" },
    { "CNET_write_direct",		std_CNET_write_direct,
	"int CNET_write_direct(CnetAddr destaddr, char *frame, int *len)" }
};

#define	NFNS		(sizeof(FUNCS) / sizeof(FUNCS[0]))


void init_physical_layer(char *Pflag, int eflag,int kflag,int Nflag,int Sflag)
{
    if(Pflag) {
	extern char	*compile_string(char *, int);
	char		*so_file;

#if !defined(USE_WIN32)
	void		*handle;
	int		fn;
#endif

	if((so_file = compile_string(Pflag, kflag)) == (char *)NULL)
	    cleanup(1);

#if !defined(USE_WIN32)
	if((handle = dlopen(so_file, RTLD_LAZY)) == NULL) {
	    fprintf(stderr,"%s: cannot load physical layer from %s\n",
					argv0, so_file);
	    exit(1);
	}
	for(fn=0 ; fn<NFNS ; fn++)
	    if((FUNCS[fn].fn = (int (*)())dlsym(handle,FUNCS[fn].fn_name)) ==
				NULL) {
		fprintf(stderr,"%s: cannot find '%s' in %s\n",
					argv0, FUNCS[fn].fn_proto, so_file);
		++nerrors;
	    }
#endif
	if(nerrors) cleanup(1);

	/* we do *not* dlclose(handle) */
    }
    (FUNCS[(int)IPL].fn)(eflag, Nflag, Sflag);
}


/* -------------------- Physical Layer Interface ------------------------ */


int reboot_physical_layer(int node)
{
    return( (FUNCS[(int)RPL].fn)(node) );
}

int clear_physical_link(int linkno)
{
    return( (FUNCS[(int)CPL].fn)(linkno) );
}

int poll_physical(CnetInt64 *when, int *node)
{
    return( (FUNCS[(int)PP].fn)(when, node) );
}


int CNET_read_physical(int *link, char *frame, int *len)
{
    if(gattr.trace_events) {
	if(VALID_INTPTR(len))
	  TRACE("\tCNET_read_physical(%s,%s,*len=%d) = ",
			find_trace_name(link), find_trace_name(frame), *len);
	else
	  TRACE("\tCNET_read_physical(%s,%s,%s) = ",
	    find_trace_name(link),find_trace_name(frame),find_trace_name(len));
    }
    if((long)len == 0L || *len <= 0)
	cnet_errno = ER_BADARG;

    else if( (FUNCS[(int)RP].fn)(link, frame, len) == 0) {
	if(gattr.trace_events)
	    TRACE("0 (link=%d,*len=%d)\n", *link, *len);
	return 0;
    }

    if(gattr.trace_events)
	TRACE("-1 %s\n", cnet_errname[(int)cnet_errno]);
    return(-1);
}


int CNET_write_physical(int link, char *frame, int *len)
{
    if(gattr.trace_events) {
	if(VALID_INTPTR(len))
	    TRACE("\tCNET_write_physical(link=%d,%s,*len=%d) = ",
			    link, find_trace_name(frame), *len);
	else
	    TRACE("\tCNET_write_physical(link=%d,%s,%s) = ",
			    link, find_trace_name(frame), find_trace_name(len));
    }
    if(NP[THISNODE].runstate == STATE_REBOOTING)
	cnet_errno = ER_NOTREADY;

    else if(link < 0 || link > NP[THISNODE].nlinks)
	cnet_errno = ER_BADLINK;

    else if((long)len == 0L || *len <= 0)
	cnet_errno = ER_BADARG;

    else if( (FUNCS[(int)WP].fn)(link, frame, len) == 0) {
	if(gattr.trace_events)
	    TRACE("0 (*len=%d)\n", *len);
	return 0;
    }
    if(gattr.trace_events)
	TRACE("-1 %s\n", cnet_errname[(int)cnet_errno]);
    return(-1);
}


/* -------------- Reliable CnetDataLink and Network layers ---------------- */


int CNET_write_physical_reliable(int link, char *frame, int *len)
{
    if(gattr.trace_events) {
	if(VALID_INTPTR(len))
	    TRACE("\tCNET_write_physical_reliable(link=%d,%s,*len=%d) = ",
				link, find_trace_name(frame), *len);
	else
	    TRACE("\tCNET_write_physical_reliable(link=%d,%s,%s) = ",
		    link, find_trace_name(frame), find_trace_name(len));
    }
    if(NP[THISNODE].runstate == STATE_REBOOTING)
	cnet_errno = ER_NOTREADY;

    else if(link < 0 || link > NP[THISNODE].nlinks)
	cnet_errno = ER_BADLINK;

    else if((long)len == 0L || *len <= 0)
	cnet_errno = ER_BADARG;

    else if( (FUNCS[(int)WPR].fn)(link, frame, len) == 0) {
	if(gattr.trace_events)
	    TRACE("0 (*len=%d)\n", *len);
	return 0;
    }

    if(gattr.trace_events)
	TRACE("-1 %s\n", cnet_errname[(int)cnet_errno]);
    return(-1);
}

int CNET_write_direct(CnetAddr destaddr, char *frame, int *len)
{
    if(gattr.trace_events) {
	if(VALID_INTPTR(len))
	      TRACE("\tCNET_write_direct(destaddr=%lu,%s,*len=%d) = ",
				    destaddr, find_trace_name(frame), *len);
	else
	      TRACE("\tCNET_write_direct(destaddr=%lu,%s,%s) = ",
			destaddr, find_trace_name(frame), find_trace_name(len));
    }
    if(NP[THISNODE].runstate == STATE_REBOOTING)
	cnet_errno = ER_NOTREADY;

    else if(destaddr == NP[THISNODE].nattr.address)
	cnet_errno = ER_BADNODE;

    else if((long)len == 0L || *len <= 0)
	cnet_errno = ER_BADARG;

    else {
	if(destaddr == ALLNODES) {
	    int n;

	    for(n=0 ; n<_NNODES ; n++)
		if(n != THISNODE && NP[n].runstate == STATE_RUNNING)
		    if( (FUNCS[(int)WD].fn)(NP[n].nattr.address,frame,len) != 0)
			goto bad_direct;
	}
	else if( (FUNCS[(int)WD].fn)(destaddr, frame, len) != 0)
		goto bad_direct;

	if(gattr.trace_events)
	    TRACE("0 (*len=%d)\n", *len);
	return 0;
    }

bad_direct:

    if(gattr.trace_events)
	TRACE("-1 %s\n", cnet_errname[(int)cnet_errno]);
    return(-1);
}


/* ------------------------------------------------------------------------ */

int CNET_set_nicaddr(int link, CnetNicaddr nicaddr)
{
    ETHERNET	*ep;
    ETHERNIC	*enp;
    LINK	*lp;
    int		n;

    if(gattr.trace_events)
	TRACE("\tCNET_set_nicaddr(link=%d,%s) = ",
		link,find_trace_name(nicaddr));

/*  ENSURE THAT A VALID LINK HAS BEEN PROVIDED */
    if(link < 1 || link > NP[THISNODE].nlinks) {
	cnet_errno = ER_BADLINK;
	goto bad;
    }

/*  ENSURE THAT THE ZERO-ADDRESS HAS NOT BEEN PROVIDED */
    for(n=0 ; n<sizeof(CnetNicaddr) ; ++n)
	if(nicaddr[n] != 0)
	    break;
    if(n == sizeof(CnetNicaddr)) {
	cnet_errno = ER_BADARG;
	goto bad;
    }

/*  ENSURE THAT THE BROADCAST-ADDRESS HAS NOT BEEN PROVIDED */
    for(n=0 ; n<sizeof(CnetNicaddr) ; ++n)
	if(nicaddr[n] != 255)
	    break;
    if(n == sizeof(CnetNicaddr)) {
	cnet_errno = ER_BADARG;
	goto bad;
    }

    lp	= &LP[ NP[THISNODE].links[link] ];

/*  ENSURE THAT WE ARE TRYING TO MODIFY AN ETHERNET LINK */
    if(lp->linktype != LT_ETHERNET) {
	cnet_errno = ER_BADLINK;
	goto bad;
    }
    ep = &ethernets[lp->endmax];

/*  CHANGE THE LOCAL COPY ON THE SEGMENT */
    for(n=0, enp=ep->nics ; n<ep->nnics ; ++n, ++enp)
	if(enp->whichnode == THISNODE) {
	    memcpy(enp->nicaddr, nicaddr, sizeof(CnetNicaddr));
	    break;
	}

/*  CHANGE THE NICADDR ON LINKATTR OF THISNODE */
    memcpy(lp->lattrmin.nicaddr, nicaddr, sizeof(CnetNicaddr));

/*  CHANGE THE NICADDR OF THE (SWAPPED IN) LINKINFO */
    memcpy(linkinfo[link].nicaddr, nicaddr, sizeof(CnetNicaddr));

    if(gattr.trace_events) {
	char	buf[32];

	CNET_format_nicaddr(buf, nicaddr);
	TRACE("0 (nicaddr=%s)\n", buf);
    }
    return 0;

bad:
    if(gattr.trace_events)
	TRACE("-1 %s\n", cnet_errname[(int)cnet_errno]);
    return(-1);
}

int CNET_set_promiscuous(int link, int now)
{
    LINK	*lp;

    if(gattr.trace_events)
	TRACE("\tCNET_set_promiscuous(link=%d,%s) = ",
			    link, now ? "TRUE" : "FALSE");

/*  ENSURE THAT A VALID LINK HAD BEEN PROVIDED */
    if(link < 1 || link > NP[THISNODE].nlinks) {
	cnet_errno = ER_BADLINK;
	goto bad;
    }

    lp	= &LP[ NP[THISNODE].links[link] ];

/*  ENSURE THAT WE ARE TRYING TO MODIFY AN ETHERNET LINK */
    if(lp->linktype != LT_ETHERNET) {
	cnet_errno = ER_BADLINK;
	goto bad;
    }

/*  CHANGE THE PROMISCUOUS MODE OF LINKATTR OF THISNODE */
    if(THISNODE == lp->endmin)
	lp->lattrmin.promiscuous = now;
    else
	lp->lattrmax.promiscuous = now;

/*  CHANGE THE PROMISCUOUS MODE OF THE (SWAPPED IN) LINKINFO */
    linkinfo[link].promiscuous	 = now;

    if(gattr.trace_events)
	TRACE("0\n");
    return 0;

bad:
    if(gattr.trace_events)
	TRACE("-1 %s\n", cnet_errname[(int)cnet_errno]);
    return(-1);
}

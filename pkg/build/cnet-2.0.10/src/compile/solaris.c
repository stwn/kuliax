#include <dlfcn.h>
#include <nlist.h>

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

static int add_compile_args(int ac, char *av[], int kflag)
{
    if(kflag)				/* using K&R "old" C compiler */
	av[ac++] =	"-PIC";
    else				/* else, GCC ANSI C compiler */
	av[ac++] =	"-fPIC";
    return(ac);
}


static int add_link_args(int ac, char *av[], int kflag)
{
    if(kflag) {				/* using K&R "old" C compiler */
	av[ac++] =	findenv("CNETCC", CNETCC);
	av[ac++] =	"cc";
	av[ac++] =	"-G";		/* create a shared object */
	av[ac++] =	"-z";		/* -assert-text */
	av[ac++] =	"text";
	av[ac++] =	"-xstrconst";
    }
    else {				/* else, GCC ANSI C compiler */
	av[ac++] =	findenv("CNETLD", CNETLD);
	av[ac++] =	"ld";
	av[ac++] =	"-G";		/* create a shared object */
	av[ac++] =	"-z";		/* -assert-text */
	av[ac++] =	"text";
    }
    return(ac);
}

static void data_segments(int n, void *handle, char *so_filenm)
{
    extern int	 	nlist(const char *, struct nlist *);
    struct nlist	nls[3];

    typedef struct _c {
	char		*so_filenm;
	unsigned long	length_data;
	char		*incore_data;
	char		*original_data;
	struct _c	*next;
    } CACHE;

    static CACHE	*chd = (CACHE *)NULL;
    CACHE		*cp  = chd;

    NODE		*np	= &NP[n];

    while(cp != (CACHE *)NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    nls[0].n_name	= "_end";
    nls[1].n_name	= "_DYNAMIC";
    nls[2].n_name	= (char *)NULL;

    if(nlist(so_filenm, nls) != 0) {
	fprintf(stderr,"%s: cannot load symbols from %s\n", argv0,so_filenm);
	++nerrors;
	return;
    }

    cp			= (CACHE *)malloc(sizeof(CACHE));
    cp->so_filenm	= strdup(so_filenm);
    cp->length_data	= (nls[0].n_value - nls[1].n_value);
    cp->incore_data	= (char *)((long)dlsym(handle,"_end")-cp->length_data);
    cp->original_data	= (char *)malloc(cp->length_data);
    memcpy(cp->original_data, cp->incore_data, cp->length_data);
    cp->next		= chd;
    chd			= cp;

    if(dflag)
	fprintf(stderr,"%s dataseg=0x%08lx len(dataseg)=%ld\n",
			    so_filenm, nls[1].n_value, cp->length_data);
found:

    np->length_data[0]		= cp->length_data;
    np->incore_data[0]		= cp->incore_data;
    np->original_data[0]	= cp->original_data;

    np->private_data[0]		= (char *)malloc(cp->length_data);
    memcpy(np->private_data[0], cp->original_data, cp->length_data);
}

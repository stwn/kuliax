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

static int add_compile_args(int ac, char *av[], int kflag)
{
    return(ac);
}


static int add_link_args(int ac, char *av[], int kflag)
{
    return(ac);
}


static void data_segments(int n, void *handle, char *so_filenm)
{
    typedef struct _c {
	char		*so_filenm;
	unsigned long	length_data[NDATASEGS];
	char		*incore_data[NDATASEGS];
	char		*original_data[NDATASEGS];
	struct _c	*next;
    } CACHE;

    static CACHE	*chd = (CACHE *)NULL;
    CACHE		*cp  = chd;
    NODE		*np	= &NP[n];
    int			i;

#if 0
    unsigned int	endat;
    unsigned int	offset;
#endif

    while(cp != (CACHE *)NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    cp			= (CACHE *)malloc(sizeof(CACHE));
    cp->so_filenm	= strdup(so_filenm);
#if 0
    if(get_ELFinfo(so_filenm, &ei) != 0) {
	fprintf(stderr,"cannot get ELFinfo\n");
	exit(1);
    }
    endat	= (int)dlsym(handle, "_end"),
    offset	= (endat - ei.endaddr);

/* FIRST, THE INITIALIZED SEGMENT */

    cp->length_data[0] = ei.datalen;
    if(ei.datalen != 0) {
	cp->incore_data[0]	= (char *)offset+ei.dataaddr;
	cp->original_data[0]	= (char *)malloc(cp->length_data[0]);
	memcpy(cp->original_data[0], cp->incore_data[0], cp->length_data[0]);
    }
    else
	cp->incore_data[0] = cp->original_data[0] = (char *)NULL;

/* THEN, THE BSS (UNINITIALIZED) SEGMENT */

    cp->length_data[1] = ei.bsslen;
    if(ei.bsslen != 0) {
	cp->incore_data[1]		= (char *)offset+ei.bssaddr;
	cp->original_data[1]	= (char *)malloc(cp->length_data[1]);
	memcpy(cp->original_data[1], cp->incore_data[1], cp->length_data[1]);
    }
    else
	cp->incore_data[1] = cp->original_data[1] = (char *)NULL;

    cp->next		= chd;
    chd			= cp;

    if(dflag) {
	fprintf(stderr,"%s:\n", so_filenm);
	fprintf(stderr,"\t  initialized=0x%08lx,   len(initialized)=%ld\n",
				(long)offset+ei.dataaddr, cp->length_data[0]);
	fprintf(stderr,"\tuninitialized=0x%08lx, len(uninitialized)=%ld\n",
				(long)offset+ei.bssaddr,  cp->length_data[1]);
    }
#endif

found:

/* FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT */

    for(i=0 ; i<NDATASEGS ; ++i) {
	np->length_data[i]	= cp->length_data[i];
	np->incore_data[i]	= cp->incore_data[i];
	np->original_data[i]	= cp->original_data[i];
	if(np->length_data[i]) {
	    np->private_data[i]	= (char *)malloc(cp->length_data[i]);
	    memcpy(np->private_data[i],cp->original_data[i],cp->length_data[i]);
	}
	else
	    np->private_data[i]	= (char *)NULL;
    }
}

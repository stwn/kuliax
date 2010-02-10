#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filehdr.h>
#include <scnhdr.h>

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

static struct {
    char	*name;
    long	size;
} section[NDATASEGS] = {
    { ".data",		0L },
    { ".sdata",		0L },
    { ".bss",		0L },
    { ".sbss",		0L },
};

static int read_sections(char *filenm)
{
    struct filehdr	filehdr;
    struct scnhdr	scnhdr;
    int			i, n, fd, nfound;

    if((fd = open(filenm,O_RDONLY,0)) < 0)
	return(1);

    if(read(fd, &filehdr, sizeof(filehdr)) <= 0	||
       lseek(fd, (off_t)filehdr.f_opthdr, SEEK_CUR) <= 0) {
	close(fd);
	return(1);
    }

    nfound	= 0;
    for(i=0 ; i<filehdr.f_nscns && nfound<NDATASEGS ; i++) {
	if(read(fd, &scnhdr, sizeof(scnhdr)) <= 0)
	    break;

	for(n=0 ; n<NDATASEGS ; ++n)
	    if(strncmp(scnhdr.s_name, section[n].name, 8) == 0) {
		section[n].size	= scnhdr.s_size;
		++nfound;
		break;
	    }
    }
    close(fd);
    return(nfound == 0);
}

static int add_compile_args(int ac, char *av[], int kflag)
{
    av[ac++] =	"-fpic";
    av[ac++] =	"-fPIC";
#if	!USE_GCC_COMPILER
    if(!kflag)
	av[ac++] = "-std1";
#endif
    return(ac);
}

static int add_link_args(int ac, char *av[], int kflag)
{
    av[ac++] =	findenv("CNETLD", CNETLD);
    av[ac++] =	"ld";
    av[ac++] =	"-d";
    av[ac++] =	"-shared";
    av[ac++] =	"-expect_unresolved";
    av[ac++] =	"*";
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

/*
    struct nlist	nls[7];
 */
    NODE		*np	= &NP[n];
    int			i;

    while(cp != (CACHE *)NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    if(read_sections(so_filenm) != 0) {
	fprintf(stderr,"%s: cannot load symbols from %s\n", argv0,so_filenm);
	++nerrors;
	return;
    }

    cp			 = (CACHE *)malloc(sizeof(CACHE));
    cp->so_filenm	 = strdup(so_filenm);

    for(i=0 ; i<NDATASEGS ; ++i) {
	cp->length_data[i]	= section[i].size;
	if(0 == cp->length_data[i])
	    continue;
	cp->incore_data[i]	= (char *)dlsym(handle, section[i].name);
	cp->original_data[i]	= (char *)malloc(cp->length_data[i]);
	memcpy(cp->original_data[i], cp->incore_data[i], cp->length_data[i]);
    }



#if 0
    nls[0].n_name	= ".data";
    nls[1].n_name	= ".rdata";
    nls[2].n_name	= ".sbss";
    nls[3].n_name	= ".bss";
    nls[4].n_name	= "CNET_DATA_ADDR";
    nls[5].n_name	= "CNET_SBSS_ADDR";
    nls[6].n_name	= (char *)NULL;

    if(nlist(so_filenm, nls) != 0) {
	fprintf(stderr,"%s: cannot load symbols from %s\n", argv0,so_filenm);
	++nerrors;
	return;
    }

    cp			 = (CACHE *)malloc(sizeof(CACHE));
    cp->so_filenm	 = strdup(so_filenm);

/* FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT */

    for(i=0 ; i<NDATASEGS ; ++i) {
	cp->length_data[i]	= (nls[2*i+1].n_value - nls[2*i].n_value);
/*
	cp->incore_data[i]	= (char *)dlsym(handle,nls[4+i].n_name) -
					nls[4+i].n_value + nls[2*i].n_value;
 */
	cp->incore_data[i]	= (char *)dlsym(handle,nls[2*i].n_name);
	cp->original_data[i]	= (char *)malloc(cp->length_data[i]);
	memcpy(cp->original_data[i], cp->incore_data[i], cp->length_data[i]);
    }
#endif

    cp->next		 = chd;
    chd			 = cp;

    if(dflag) {
	fprintf(stderr,"%s:\n", so_filenm);
	for(i=0 ; i<NDATASEGS ; ++i)
	    fprintf(stderr,"\t  %6s=0x%012lx len(%6s)=%ld\n",
			section[i].name, (long)cp->incore_data[i],
			section[i].name,  cp->length_data[i]);
    }

found:

/* FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT */

    for(i=0 ; i<NDATASEGS ; ++i) {
	np->length_data[i]	= cp->length_data[i];
	if(0 == cp->length_data[i])
	    continue;
	np->incore_data[i]	= cp->incore_data[i];
	np->original_data[i]	= cp->original_data[i];
	np->private_data[i]	= (char *)malloc(cp->length_data[i]);
	memcpy(np->private_data[i], cp->original_data[i], cp->length_data[i]);
    }
}

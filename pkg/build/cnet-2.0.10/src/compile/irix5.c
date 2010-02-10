#include <dlfcn.h>
#include <nlist.h>
#include <libelf.h>

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
#if	!USE_GCC_COMPILER
    av[ac++] =	"-woff";
    av[ac++] =	"636,653,740,771";
#endif
    return(ac);
}


static int add_link_args(int ac, char *av[], int kflag)
{
    av[ac++] =	findenv("CNETLD", CNETLD);
    av[ac++] =	"ld";
    av[ac++] =	"-elf";
    av[ac++] =	"-shared";
    av[ac++] =	"-d";
    return(ac);
}


static int elf_sizes(char *fn, unsigned long *datalen, unsigned long *bsslen)
{
    int		fd;
    int		found = 0;
    char	*sname;
    Elf		*elf;
    Elf32_Ehdr	*ehdr;
    Elf_Scn	*scn;
    Elf32_Shdr	*shdr;

    if((fd = open(fn, O_RDONLY, 0)) < 0) {
	fprintf(stderr,"%s: cannot open %s\n", argv0, fn);
	return(1);
    }

    elf_version(EV_CURRENT);
    elf = elf_begin(fd, ELF_C_READ, (Elf *)NULL);
    if(elf == (Elf *)NULL || elf_kind(elf) != ELF_K_ELF) {
	fprintf(stderr,"%s: %s is not in ELF format\n", argv0, fn);
	close(fd);
	return(1);
    }

    ehdr = elf32_getehdr(elf);
    scn	= (Elf_Scn *)NULL;

    while((scn = elf_nextscn(elf, scn)) != 0) {
	shdr	= elf32_getshdr(scn);
	sname	= elf_strptr(elf, ehdr->e_shstrndx, (size_t)shdr->sh_name);

	if(sname != (char *)NULL) {
	    if(strcmp(sname, ELF_DATA) == 0) {
		*datalen	= (unsigned long)shdr->sh_size;
		if(++found == 2)
		    break;
	    }
	    else if(strcmp(sname, ELF_BSS) == 0) {
		*bsslen		= (unsigned long)shdr->sh_size;
		if(++found == 2)
		    break;
	    }
	}
    }
    elf_end(elf);
    close(fd);
    return(0);
}


static void data_segments(int n, void *handle, char *so_filenm)
{
    extern int	 	nlist(const char *, struct nlist *);

    typedef struct _c {
	char		*so_filenm;
	unsigned long	length_data[NDATASEGS];
	char		*incore_data[NDATASEGS];
	char		*original_data[NDATASEGS];
	struct _c	*next;
    } CACHE;

    static CACHE	*chd = (CACHE *)NULL;
    CACHE		*cp  = chd;
    struct nlist	nls[3];
    NODE		*np	= &NP[n];
    int			i;

    while(cp != (CACHE *)NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    nls[0].n_name	= ELF_DATA;
    nls[1].n_name	= ELF_BSS;
    nls[2].n_name	= (char *)NULL;

    if(nlist(so_filenm, nls) != 0) {
	fprintf(stderr,"%s: cannot load symbols from %s\n", argv0,so_filenm);
	exit(1);
    }

    cp			= (CACHE *)malloc(sizeof(CACHE));
    cp->so_filenm	= strdup(so_filenm);
    if(elf_sizes(so_filenm, &(cp->length_data[0]), &(cp->length_data[1])) != 0)
	exit(1);

/* FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT */

    for(i=0 ; i<NDATASEGS ; ++i) {
	if(nls[i].n_value) {
	    cp->incore_data[i]		= (char *)nls[i].n_value;
	    cp->original_data[i]	= (char *)malloc(cp->length_data[i]);
	    memcpy(cp->original_data[i],
			    cp->incore_data[i], cp->length_data[i]);
	}
	else {
	    cp->length_data[i] = 0;
	    cp->incore_data[i] = cp->original_data[i] = (char *)NULL;
	}
    }
    cp->next		= chd;
    chd			= cp;

    if(dflag) {
	fprintf(stderr,"%s:\n", so_filenm);
	fprintf(stderr,"\t  initialized=0x%08lx,   len(initialized)=%ld\n",
				nls[0].n_value, cp->length_data[0]);
	fprintf(stderr,"\tuninitialized=0x%08lx, len(uninitialized)=%ld\n",
				nls[1].n_value, cp->length_data[1]);
    }

found:

/* FIRST, THE INITIALIZED SEGMENT, THEN THE UNINITIALIZED SEGMENT */

    for(i=0 ; i<NDATASEGS ; ++i) {
	np->length_data[i]	= cp->length_data[i];
	np->incore_data[i]	= cp->incore_data[i];
	np->original_data[i]	= cp->original_data[i];
	if(np->length_data[i]) {
	    np->private_data[i]	= (char *)malloc(cp->length_data[i]);
	    memcpy(np->private_data[i],
			    cp->original_data[i], cp->length_data[i]);
	}
	else
	    np->private_data[i]	= (char *)NULL;
    }
}

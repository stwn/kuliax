#include <fcntl.h>
#include <libelf.h>
#include <dlfcn.h>

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
    av[ac++]	= "-rdynamic";
    av[ac++]	= "-fPIC";
    return(ac);
}


static int add_link_args(int ac, char *av[], int kflag)
{
    av[ac++] =	findenv("CNETLD", CNETLD);
    av[ac++] =	"gcc";
    av[ac++] =	"-shared";
    return(ac);
}


#define	ELF_DATA_SYMBOL		".data"
#define	ELF_BSS_SYMBOL		".bss"
#define	ELF_END_SYMBOL		"_end"


typedef struct {
    unsigned int	datalen;
    unsigned int	bsslen;
    unsigned int	dataaddr;
    unsigned int	bssaddr;
    unsigned int	endaddr;
} ELFinfo;


/*  The following function borrows heavily from the sourcefile nlist.c
    of the public release of libelf-0.6.4,
    a free ELF object file access library,
    written by Michael "Tired" Riepe <michael@stud.uni-hannover.de>
    available from	http://www.stud.uni-hannover.de/~michael/software/
 */

static int get_ELFinfo(char *filenm, ELFinfo *ei)
{
    int		fd, i;
    int		found=0, rtn=1;
    int		nsymbols, nstrings;
    char	*sname, *strings;

    Elf		*elf;
    Elf32_Ehdr	*ehdr;
    Elf32_Shdr	*shdr;
    Elf_Scn	*scn;
    Elf_Scn	*strtab = NULL;
    Elf_Scn	*symtab = NULL;
    Elf32_Sym	*symbols;
    Elf_Data	*strdata, *symdata;

    if((fd = open(filenm, O_RDONLY, 0)) < 0) {
	fprintf(stderr,"%s: cannot open %s\n", argv0, filenm);
	return(1);
    }
    elf_version(EV_CURRENT);
    elf = elf_begin(fd, ELF_C_READ, (Elf *)NULL);
    if(elf == (Elf *)NULL || elf_kind(elf) != ELF_K_ELF) {
	fprintf(stderr,"%s: %s is not in ELF format\n", argv0,filenm);
	close(fd);
	return(1);
    }

    ehdr = elf32_getehdr(elf);
    scn	= (Elf_Scn *)NULL;

    while((scn = elf_nextscn(elf, scn)) != 0) {
	shdr	= elf32_getshdr(scn);
	sname	= elf_strptr(elf, ehdr->e_shstrndx, (size_t)shdr->sh_name);

	if(sname != (char *)NULL) {
	    if(strcmp(sname, ELF_DATA_SYMBOL) == 0) {
		ei->dataaddr	= (unsigned int)shdr->sh_addr;
		ei->datalen	= (unsigned int)shdr->sh_size;
		if(++found == 2)
		    break;
	    }
	    else if(strcmp(sname, ELF_BSS_SYMBOL) == 0) {
		ei->bssaddr	= (unsigned int)shdr->sh_addr;
		ei->bsslen	= (unsigned int)shdr->sh_size;
		if(++found == 2)
		    break;
	    }
	}
    }
    if(found != 2)
	goto done;

/*  WE COULD USE THE STANDARD nlist() FUNCTION HERE, BUT WE ALREADY HAVE
    THE SHARED LIBRARY OPENED, SO WE JUST USE THE "RAW" LIBELF ROUTINES.
 */

    scn	= (Elf_Scn *)NULL;				/* rewind ELF file */
    while((scn = elf_nextscn(elf, scn)) != 0) {
	shdr	= elf32_getshdr(scn);
	sname	= elf_strptr(elf, ehdr->e_shstrndx, (size_t)shdr->sh_name);

	if(shdr->sh_type != SHT_SYMTAB && shdr->sh_type != SHT_DYNSYM) {
	    continue;
	}
	symtab = scn;
	strtab = elf_getscn(elf, shdr->sh_link);
	if(shdr->sh_type == SHT_SYMTAB)
	    break;
    }
    symdata = elf_getdata(symtab, NULL);
    strdata = elf_getdata(strtab, NULL);
    if(!symdata || !strdata)
	goto done;

    symbols = (Elf32_Sym*)symdata->d_buf;
    strings = (char*)strdata->d_buf;
    nsymbols = symdata->d_size / sizeof(Elf32_Sym);
    nstrings = strdata->d_size;
    if(!symbols || !strings || !nsymbols || !nstrings)
	goto done;

    for(i=1; i<nsymbols; i++) {
	if(symbols[i].st_name < 0 || symbols[i].st_name >= nstrings)
	    goto done;
	if(symbols[i].st_name == 0) {
	    continue;
	}
	sname = strings + symbols[i].st_name;
	if(strcmp(sname, ELF_END_SYMBOL) == 0) {
	    ei->endaddr	= symbols[i].st_value;
	    break;
	}
    }
    rtn	= 0;
done:
    elf_end(elf);
    close(fd);
    return(rtn);
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

    ELFinfo		ei;
    unsigned int	endat;
    unsigned int	offset;

    while(cp != (CACHE *)NULL) {
	if(strcmp(cp->so_filenm, so_filenm) == 0)
	    goto found;
	cp	= cp->next;
    }

    cp			= (CACHE *)malloc(sizeof(CACHE));
    cp->so_filenm	= strdup(so_filenm);
    errno		= 0;
    if(get_ELFinfo(so_filenm, &ei) != 0) {
	fprintf(stderr,"cannot get ELFinfo\n");
	if(errno != 0)
	    perror(argv0);
	exit(1);
    }
    endat	= (int)dlsym(handle, "_end"),
    offset	= (endat - ei.endaddr);

/* FIRST, THE INITIALIZED SEGMENT */

    cp->length_data[0] = ei.datalen;
    if(ei.datalen != 0) {
	cp->incore_data[0]		= (char *)offset+ei.dataaddr;
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

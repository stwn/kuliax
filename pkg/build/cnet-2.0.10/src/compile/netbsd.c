#include <fcntl.h>
#include <elf.h>
#include <nlist.h>
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

/*  Sections of this NetBSD code were contributed by Mark Davies,
    Victoria University of Wellington, NZ   <mark@mcs.vuw.ac.nz>
    Wed, 15 Aug 2001.
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
    av[ac++] =	"ld";
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


/*  The following function borrows heavily from the source of elf2aout.c
    from the NetBSD-1.5 sources.
 */
static char   *
saveRead(int file, off_t offset, off_t len, char *name)
{
        char   *tmp;
        int     count;
        off_t   off;
        if ((off = lseek(file, offset, SEEK_SET)) < 0) {
                fprintf(stderr, "%s: fseek: %s\n", name, strerror(errno));
                exit(1);
        }
        if (!(tmp = (char *) malloc(len))) {
                fprintf(stderr, "%s: Can't allocate %ld bytes.", name,
			(long)len);
		exit(1);
	}
        count = read(file, tmp, len);
        if (count != len) {
                fprintf(stderr, "%s: read: %s.\n",
                    name, count ? strerror(errno) : "End of file reached");
                exit(1);
        }
        return tmp;
}

static int get_ELFinfo(char *filenm, ELFinfo *ei)
{
    int		fd, i;
    int		found=0, rtn=1;
    int         nsymbols, nstrings;
    int		strtabix, symtabix;
    char	*sname, *strings;
    

    Elf32_Ehdr	ex;
    Elf32_Shdr	*sh;
    char   	*shstrtab;
    Elf32_Sym   *symbols;


    if((fd = open(filenm, O_RDONLY, 0)) < 0) {
	fprintf(stderr,"%s: cannot open %s\n", argv0, filenm);
	return(1);
    }
    /* Read the header, which is at the beginning of the file... */
    i = read(fd, &ex, sizeof ex);
    if (i != sizeof ex) {
      fprintf(stderr, "%s: ex: %s.\n",
	      argv0, i ? strerror(errno) : "End of file reached");
      close(fd);
      return(1);
    }

    /* Read the section headers... */
    sh = (Elf32_Shdr *) saveRead(fd, ex.e_shoff,
				 ex.e_shnum * sizeof(Elf32_Shdr), "sh");
    /* Read in the section string table. */
    shstrtab = saveRead(fd, sh[ex.e_shstrndx].sh_offset,
			sh[ex.e_shstrndx].sh_size, "shstrtab");

    for (i = 0; i < ex.e_shnum; i++) {
        sname = shstrtab + sh[i].sh_name;
 
        if(sname != (char *)NULL) {
	    if(strcmp(sname, ELF_DATA_SYMBOL) == 0) {
		ei->dataaddr	= (unsigned int)sh[i].sh_addr;
		ei->datalen	= (unsigned int)sh[i].sh_size;
		if(++found == 2)
		    break;
	    }
	    else if(strcmp(sname, ELF_BSS_SYMBOL) == 0) {
		ei->bssaddr	= (unsigned int)sh[i].sh_addr;
		ei->bsslen	= (unsigned int)sh[i].sh_size;
		if(++found == 2)
		    break;
	    }
	}
    }
    if(found != 2)
	goto done;


 
    /* Look for the symbol table and string table... Also map section
     * indices to symbol types for a.out */
    for (i = 0; i < ex.e_shnum; i++) {
      sname = shstrtab + sh[i].sh_name;
      if (!strcmp(sname, ".symtab"))
	symtabix = i;
      else
	if (!strcmp(sname, ".strtab"))
	  strtabix = i;
      
    }
    symbols = (Elf32_Sym*)saveRead(fd, sh[symtabix].sh_offset,
				   sh[symtabix].sh_size, "symbol table");
    strings = saveRead(fd, sh[strtabix].sh_offset, sh[strtabix].sh_size,
		       "string table");
    nsymbols = sh[symtabix].sh_size / sizeof(Elf32_Sym);
    nstrings = sh[strtabix].sh_size;
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
            ei->endaddr = symbols[i].st_value;
            break;
        }
    }
    rtn	= 0;
done:
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

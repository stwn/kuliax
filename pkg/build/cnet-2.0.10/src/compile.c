#include "cnetheader.h"
#include <fcntl.h>
#include <sys/stat.h>

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

#if	!defined(MAXPATHLEN)
#define	MAXPATHLEN	1024
#endif

static	char	**linkerflags	= (char **)NULL;
static	int	nlinkerflags	= 0;


#if	defined(USE_LINUX)
#include "compile/linux.c"

#elif	defined(USE_OSF1)
#include "compile/osf1.c"

#elif	defined(USE_SUNOS)
#include "compile/sunos.c"

#elif	defined(USE_SOLARIS)
#include "compile/solaris.c"

#elif	defined(USE_IRIX5)
#include "compile/irix5.c"

#elif	defined(USE_FREEBSD)
#include "compile/freebsd.c"

#elif	defined(USE_NETBSD)
#include "compile/netbsd.c"

#elif	defined(USE_WIN32)
#include <process.h>
#include "compile/win32.c"

#endif


/* -------------------------------------------------------------------- */

static void dynamic_load(int n, char *so_file)
{
#if defined(USE_WIN32)
    /* help */
#else

#if	defined(RTLD_NOW)
    void *handle	= dlopen(so_file, RTLD_NOW);
#else
    void *handle	= dlopen(so_file, RTLD_LAZY);
#endif

    if(handle == NULL) {
	fprintf(stderr,"%s: NULL HANDLE from \"%s\"!\n",argv0,so_file);
	fprintf(stderr,"Why: %s\n", dlerror() );
	++nerrors;
    }
    else if((NP[n].handler[(int)EV_REBOOT] =
	    (void (*)())dlsym(handle, NP[n].nattr.reboot_func)) == NULL) {
	fprintf(stderr,"%s: cannot find %s() in \"%s\"\n", argv0,
				NP[n].nattr.reboot_func, so_file);
	++nerrors;
    }
    else
	data_segments(n, handle, so_file);  /* we do *not* dlclose(handle) */
#endif
}


void compile_topology(int kflag)
{
    extern char	*compile_string(char *, int);

    char	*so_file;
    int		n;

#if	defined(APPEND_DOT_TO_LDPATH)
    {
	extern int	putenv(char *);
	char		*newenv, *oldenv;

	if((oldenv = findenv("LD_LIBRARY_PATH", NULL)) == (char *)NULL)
	    newenv	= strdup("LD_LIBRARY_PATH=\".\"");
	else {
	    sprintf(chararray,"LD_LIBRARY_PATH=\"%s:.\"", oldenv);
	    newenv	= strdup(chararray);
	}
	putenv(newenv);
    }
#endif

    for(n=0 ; n<_NNODES ; n++) {
	so_file	= compile_string(NP[n].nattr.compile, kflag);
	if(nerrors)
	    cleanup(1);
	dynamic_load(n, so_file);
	if(nerrors)
	    cleanup(1);
    }
}


char *find_cnetfile(char *filenm, int wantdir, int fatal)
{
    char	ch;
    char	*env, *orig, *p, *pathname;

/*  FIRSTLY, TRY TO FIND THE ABSOLUTE filenm */

    if(*filenm == '/') {
	if(access(filenm, R_OK) == 0)
	    return( strdup(filenm) );
    }
    else {
/*  ELSE, TRY TO FIND filenm USING $CNETPATH */

	env	 = findenv("CNETPATH", CNETPATH);
	orig = env	= strdup(env);

	while(*env) {
	    while(*env == PATH_SEPARATOR)
		++env;
	    p	= env;
	    while(*p && *p != PATH_SEPARATOR)
		++p;
	    ch	= *p;
	    *p	= '\0';
	    sprintf(chararray, "%s/%s", env, filenm);	/* find this! */
	    if(access(chararray, R_OK) == 0) {
		pathname	= strdup(wantdir ? env : chararray);
		free(orig);
		return(pathname);
	    }
	    *p	= ch;
	    env	= p;
	}
	free(orig);
    }
    if(fatal) {
	fprintf(stderr,"%s: cannot locate \"%s\"\n", argv0,filenm);
	cleanup(1);				/* will not return */
    }
    return((char *)NULL);
}


/* -------------------------------------------------------------------- */

#if	CHECK_RECEIVE_SPELLING
static int spell_check(char *filenm)
{
    FILE	*fp;
    char	*s;
    int		errs=0, lc=0;

    if((fp = fopen(filenm,"r")) != (FILE *)NULL) {
	for(;;) {
	    fgets(chararray,BUFSIZ-1,fp);
	    if(feof(fp))
		break;
	    lc++;
	    s	= chararray;
	    while(*s) {
		if(isupper((int)*s))
		    *s	= tolower((int)*s);
		++s;
	    }
	    if(strstr(chararray, "reciev") || strstr(chararray, "interup")) {
		fprintf(stderr,"%s: spelling mistake on line %d of %s\n",
				argv0, lc, filenm);
		++nerrors; ++errs;
	    }
	}
	fclose(fp);
    }
    return(errs == 0 ? TRUE : FALSE);
}
#endif


/* -------------------------------------------------------------------- */

static int make_ofile(int kflag, char **Cflags, char *o_file, char *c_file)
{
#if	!defined(USE_WIN32)
    int		pid;
#endif

    struct stat	stat_c, stat_o;

    static char	*cnp	= (char *)NULL;
    char	*av[64];		/* hope that this is enough! */
    int		status;
    int		ac, i;

/*  FIRSTLY, ENSURE THAT THE SOURCEFILE EXISTS AND NEEDS RECOMPILING */

    if(stat(c_file, &stat_c) == -1) {
       fprintf(stderr,"%s: cannot find sourcefile %s\n",argv0,c_file);
	++nerrors;
	return(-1);
    }
    if(stat(o_file, &stat_o) == 0 && stat_c.st_mtime <= stat_o.st_mtime)
	return(0);

#if	CHECK_RECEIVE_SPELLING
    if(spell_check(c_file) == FALSE)
	return(-1);
#endif

    if(cnp == (char *)NULL) {
	cnp	= find_cnetfile("cnet.h", TRUE, TRUE);
	if(vflag)
	    fprintf(stderr,"using include directory \"%s\"\n", cnp);
    }

#if defined(USE_WIN32)
    ac	= 0;

    av[ac++] = "CL";
    av[ac++] = OS_DEFINE;

    av[ac++] =	"/DHAVE_LONG_LONG=1";
    sprintf(chararray, "/DSIZEOF_INT=%d",	sizeof(int));
    av[ac++] =	strdup(chararray);
    sprintf(chararray, "/DSIZEOF_LONG=%d",	sizeof(long));
    av[ac++] =	strdup(chararray);

    sprintf(chararray, "/I%s", cnp);
    av[ac++] = strdup(chararray);

    while(*Cflags)		/* add C compiler switches */
	av[ac++] =	*Cflags++;

    av[ac++] = "/c";
    sprintf(chararray, "/Fo%s", o_file);
    av[ac++] = strdup(chararray);

    if(!vflag)
	av[ac++] = "/NOLOGO";

    av[ac++] = c_file;
    av[ac  ] =	NULL;

    if(dflag) {
	fputs(av[0], stderr);
	for(i=1 ; i<ac ; i++)
	    fprintf(stderr," %s",av[i]);
	fputc('\n',stderr);
    }
    else
	fprintf(stderr,"compiling %s\n", c_file);

    status  = _spawnvp(_P_WAIT, av[0], &av[1]);
    if(status != 0) {
	if(status == -1)
	    fprintf(stderr,"%s: spawn of %s unsuccessful: %s\n",
				argv0,av[0],_sys_errlist[(int)errno]);
	exit(1);
    }

#else
    switch (pid = fork()) {
    case -1 :
	fprintf(stderr,"%s: cannot fork\n",argv0);
	exit(1);
	break;
    case 0 :
	ac	 = 0;

#if	USE_GCC_COMPILER
	av[ac++] = findenv("CNETGCC", CNETGCC);
	av[ac++] = "gcc";
	if(!kflag)			/* not using "old" K&R C */
	    av[ac++] = "-ansi";
#if	GCC_WERROR_WANTED
	av[ac++] = "-Werror";
#endif
#if	GCC_WALL_WANTED
	av[ac++] = "-Wall";
#endif

#else
	av[ac++] = findenv("CNETCC", CNETCC);
	av[ac++] = "cc";
#endif

	ac	 =	add_compile_args(ac, av, kflag);
	av[ac++] =	OS_DEFINE;

#if	HAVE_LONG_LONG
	av[ac++] =	"-DHAVE_LONG_LONG=1";
#endif
	sprintf(chararray, "-DSIZEOF_INT=%d",	sizeof(int));
	av[ac++] =	strdup(chararray);
	sprintf(chararray, "-DSIZEOF_LONG=%d",	sizeof(long));
	av[ac++] =	strdup(chararray);

	while(*Cflags)		/* add C compiler switches */
	    av[ac++] =	*Cflags++;

	sprintf(chararray, "-I%s", cnp);
	av[ac++] =	strdup(chararray);

	av[ac++] =	"-c";
	av[ac++] =	"-o";
	av[ac++] =	o_file;
	av[ac++] =	c_file;
	av[ac  ] =	NULL;

	if(dflag) {
	    fputs(av[0], stderr);
	    for(i=2 ; i<ac ; i++)
		fprintf(stderr," %s",av[i]);
	    fputc('\n',stderr);
	}
	else
	    fprintf(stderr,"compiling %s\n", c_file);

	execvp(av[0], &av[1]);
	fprintf(stderr,"%s: cannot exec %s\n",argv0,av[0]);
	exit(1);
        break;

    default :
	while(wait(&status) != pid)
	    ;
	if(status != 0)
	    exit(1);
	break;
    }
#endif
    return(0);
}


static int make_sofile(int kflag, char *so_file, char **o_files)
{
#if	!defined(WIN32)
    int		pid;
#endif
    extern int	unlink(const char *);

    struct stat	stat_o, stat_so;

    int		status;
    int		ac, i;
    char	**ofs;
    char	*av[64];		/* hope that this is enough! */

    if(stat(so_file, &stat_so) == 0) {		/* hmmm, so_file exists */
	ofs	= o_files;
	while(*ofs) {
	    if(stat(*ofs, &stat_o) == -1 || stat_so.st_mtime <= stat_o.st_mtime)
		break;
	    ++ofs;
	}
	if(*ofs == (char *)NULL)
	    return(0);
    }

#if	defined(USE_WIN32)
    ac	 =	0;
    av[ac++] =	"LINK";
    av[ac++] =	"/FORCE:UNRESOLVED";
    av[ac++] =	"/DLL";

    sprintf(chararray, "/OUT:%s", so_file);
    av[ac++] =	strdup(chararray);

    for(i=0 ; i<nlinkerflags ; ++i)		/* add any -L or -l switches */
	av[ac++] =	linkerflags[i];

    if(!vflag)
	av[ac++] = "/NOLOGO";

    ofs	 =	o_files;
    while(*ofs)
	av[ac++] = *ofs++;

    av[ac  ] =	(char *)NULL;

    if(dflag) {
	fputs(av[0], stderr);
	for(i=2 ; i<ac ; i++)
	    fprintf(stderr," %s",av[i]);
	fputc('\n',stderr);
    }
    else
	fprintf(stderr,"linking %s\n", so_file);

    status  = _spawnv(_P_WAIT, av[0], &av[1]);
    if(status != 0) {
	if(status == -1)
	    fprintf(stderr,"%s: spawn of %s unsuccessful: %s\n",
				argv0,av[0],_sys_errlist[(int)errno]);
	exit(1);
    }

#else
    unlink(so_file);
    switch (pid = fork()) {
    case -1 :
	fprintf(stderr,"%s: cannot fork\n",argv0);
	exit(1);
	break;
    case 0 :
	ac	 =	0;
	ac	 =	add_link_args(ac, av, kflag);
	av[ac++] =	"-o";
	av[ac++] =	so_file;

	ofs	 =	o_files;
	while(*ofs)
	    av[ac++] =	*ofs++;

	for(i=0 ; i<nlinkerflags ; ++i)	/* add any -L or -l switches */
	    av[ac++] =	linkerflags[i];
	av[ac  ] =	(char *)NULL;

	if(dflag) {
	    fputs(av[0], stderr);
	    for(i=2 ; i<ac ; i++)
		fprintf(stderr," %s",av[i]);
	    fputc('\n',stderr);
	}
	else
	    fprintf(stderr,"linking %s\n", so_file);

	execvp(av[0], &av[1]);
	fprintf(stderr,"%s: cannot exec %s\n",argv0,av[0]);
	exit(1);
        break;
    default :
	while(wait(&status) != pid)
	    ;
	break;
    }
#endif

#if	defined(UNLINK_SO_LOCATIONS)
    unlink("so_locations");
#endif
    if(status != 0) {
	unlink(so_file);
	exit(1);
    }
    return(0);
}


/* -------------------------------------------------------------------- */

char *compile_string(char *str, int kflag)
{
    static char	*laststr	= (char *)NULL;
    static char	*so_file	= (char *)NULL;

    char	savech, *dot, *slash;
    char	*word, *cp, *c_files;
    char	*filenm, **ofs, **o_files;

    char	**Cflags;
    int		nCflags	= 0;
    int		n;

    if(laststr && strcmp(str,laststr) == 0)	/* identical to last node */
	return(so_file);

    if(nlinkerflags) {
	for(n=0 ; n<nlinkerflags ; ++n)
	    free(linkerflags[n]);
	free((char *)linkerflags);
	nlinkerflags	= 0;
    }
    laststr	= str;
    so_file	= (char *)NULL;

    cp 	= c_files	= strdup(str);
    n	  		= (int)strlen(str)/3;
    ofs	= o_files	= (char **)malloc(n * sizeof(char *));
    Cflags		= (char **)malloc(n * sizeof(char *));
    linkerflags		= (char **)malloc(n * sizeof(char *));

    while(*cp) {
/*  SKIP LEADING BLANKS */
	while(*cp == ' ' || *cp == '\t')
	    ++cp;

	word	= cp;
	while(*cp && *cp != ' ' && *cp != '\t')
	    ++cp;

	if(cp != word) {
	    savech	= *cp;
	    *cp		= '\0';

/* FIRSTLY, CHECK FOR ANY COMPILER OR LINKER SWITCHES */
	    if(*word == OPTION_CH) {
		switch (*(word+1)) {
		    case 'l' :
		    case 'L' :	linkerflags[nlinkerflags++] = strdup(word);
				break;
		    default  :	Cflags[nCflags++] = strdup(word);
				break;
		}
		*cp	= savech;
		continue;
	    }

/* IF NOT A SWITCH, THEN ENSURE THAT WE HAVE A C SOURCE FILE */
	    if((dot=strrchr(word,'.'))==(char *)NULL || strcmp(dot,".c") != 0) {
		fprintf(stderr,"%s: sourcefile %s must be a C file\n",
					    argv0, word);
		++nerrors;
		break;
	    }
	    *dot	= '\0';

	    filenm	= (slash = strrchr(word,'/')) ? slash+1 : word;
	    *ofs	= (char *)malloc(strlen(filenm) + 6);
#if	defined(USE_WIN32)
	    sprintf(*ofs, "%s.obj", filenm);
#else
	    sprintf(*ofs, "%s.o", filenm);
#endif

	    *dot		= '.';
	    Cflags[nCflags]	= (char *)NULL;

/* NEXT, TRY AND COMPILE EACH C FILE */
	    if(make_ofile(kflag, Cflags, *ofs, word) != 0)
		break;
	    ++ofs;
	    *cp		= savech;
	}
    }
    *ofs	= (char *)NULL;

/* FINALLY, LINK TOGETHER ALL OBJECT FILES INTO A SHARED OBJECT FILE */
    if(nerrors == 0) {
	cp	= c_files;

	while(*cp) {
	    while(*cp == ' ' || *cp == '\t')		/* skip blanks */
		++cp;
	    if(*cp != OPTION_CH)			/* found a pathname */
		break;
	    while(*cp && *cp != ' ' && *cp != '\t')	/* skip switch */
		++cp;
	}

	word	= cp;
	while(*cp && *cp != ' ' && *cp != '\t')	/* skip to end of pathname */
	    ++cp;
	*cp		= '\0';
	filenm 		= (slash = strrchr(word,'/')) ? slash+1 : word;
	dot		= strrchr(filenm,'.');
	*dot		= '\0';

	so_file		= (char *)malloc((cp - filenm) + 12);

#if	defined(PREPEND_DOT_TO_SO_SOFILE)
	sprintf(so_file, "./%s.cnet", filenm);
#else
	sprintf(so_file, "%s.cnet", filenm);
#endif

	linkerflags[nlinkerflags]	= (char *)NULL;

	if(make_sofile(kflag, so_file, o_files) != 0) {
	    ++nerrors;
	    free(so_file);
	    so_file	= (char *)NULL;
	}
    }

    ofs	= o_files;
    while(*ofs)
	free(*ofs++);
    free((char *)o_files);
    free((char *)c_files);
    free((char *)Cflags);

    return(so_file);
}

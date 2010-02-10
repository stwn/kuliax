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

#if	!defined(MAXPATHLEN)
#define	MAXPATHLEN	1024
#endif


/*  The motd() function will be called early from main() to give you
    a chance to provide any message-of-the-day to students.  Examples may
    include any recently discovered problems (?) or modifications to
    project requirements.  The code provided below (and compiled if
    MOTD_WANTED is TRUE) ensures that students' cnet projects are being
    developed in their $HOME/it312/cnetproj directories - a feature which I
    enable close to project deadlines to assist automatic project collection.

    #define	CHECK_LOCATION
 */


#define	MOTD_FILE	"/cslinux/examples/it312/cnet.motd"
#define	PROJ_DIR	"it312/cnetproj"
#define	CHECK_LOCATION


void motd()			/* called from cnetmain.c:main() */
{
#if	MOTD_WANTED

#if     defined(__GNUC__)
    extern char *getcwd(char *, size_t);
#else
    extern char	*getwd(char *);
#endif

    char	line[BUFSIZ];
    FILE	*fp;
#if	defined(CHECK_LOCATION)
    char	pathname[MAXPATHLEN];
#endif

    if((fp = fopen(MOTD_FILE, "r")) != (FILE *)NULL) {
	for(;;) {
	    fgets(line, BUFSIZ, fp);
	    if(feof(fp) || ferror(fp))
		break;
	    fputs(line, stdout);
	}
	fclose(fp);
    }

#if	defined(CHECK_LOCATION)

    sprintf(chararray, "%s/%s", findenv("HOME", "?"), PROJ_DIR);
#if     defined(__GNUC__)
    if(getcwd(pathname,MAXPATHLEN) == (char *)NULL ||
#else
    if(getwd(pathname) == (char *)NULL ||
#endif

    strncmp(chararray, pathname, strlen(chararray)) != 0) {
	fprintf(stderr,"\007Your project should be in your ");
	fprintf(stderr,"'%s'\n", chararray);
	fprintf(stderr,"directory.  Please move your files there now.\n\n");
/*
	exit(1);
 */
    }
#endif
#endif
}

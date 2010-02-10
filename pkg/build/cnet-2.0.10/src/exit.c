#include "cnetheader.h"

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

void CNET_exit(const char *filenm, const char *function, int lineno)
{
    NODE	*np = &NP[THISNODE];

    if(function)
	sprintf(chararray,
	    "Error while executing %s, file %s, function %s(), line %d :\n%s",
	    np->nodename, filenm, function, lineno, cnet_errstr[cnet_errno]);
    else
	sprintf(chararray,
	    "Error while executing %s, file %s, line %d :\n%s",
	    np->nodename, filenm, lineno, cnet_errstr[cnet_errno]);

    fprintf(stderr,"%s\n", chararray);

#if defined(USE_TCLTK)
    if(Wflag) {
	char *str	= strdup(chararray);

	TCLTK("show_error \"%s\" \"%s\" %d", str, filenm, lineno);
    }
#endif
    exit(1);
}

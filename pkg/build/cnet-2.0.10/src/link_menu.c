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

#if	defined(USE_TCLTK)

static char	*menu_strings[] = {
	"Sever",
	"Reconnect",
	"?",				/* set in *_init_linkmenu() */
	"?"				/* set in *_init_linkmenu() */
};

#define	N_LINKMENU_STRINGS	(sizeof(menu_strings) / sizeof(menu_strings[0]))

#define	SE			0
#define	RE			1
#define	SSHORT			2
#define	SLONG			3

#define	SEMASK			(1<<SE)
#define	REMASK			(1<<RE)
#define	SSHORTMASK		(1<<SSHORT)
#define	SLONGMASK		(1<<SLONG)


static int linkmenu_mask(int l)
{
    if(LP[l].linkup)
	return(SEMASK | SSHORTMASK | SLONGMASK);
    else
	return(REMASK);
}


static int linkmenu_select(ClientData data, Tcl_Interp *interp,
				int argc, char *argv[])
{
    extern int	clear_physical_link(int);
    extern void report_linkstate(int);

    CnetInt64	tmp64;
    int		l, s;

    if(vflag) {
	for(l=0 ; l<argc-1 ; ++l)
	    fprintf(stderr,"%s ", argv[l]);
	fprintf(stderr,"%s\n", argv[argc-1]);
    }
    if(argc != 3) {
	interp->result	= "wrong # args";
	return TCL_ERROR;
    }
    l	= atoi(argv[1]);
    s	= atoi(argv[2]);
    if(l < 0 || l >= _NLINKS || s < 0 || s >= N_LINKMENU_STRINGS) {
	interp->result	= "invalid link or menu arg";
	return TCL_ERROR;
    }

    switch (s) {
    case SE :		LP[l].linkup		= FALSE;
			clear_physical_link(l);
			break;
    case RE :		LP[l].linkup		= TRUE;
			break;
    case SSHORT :
    case SLONG :	LP[l].linkup	        = FALSE;
			clear_physical_link(l);
			if(s == SSHORT) {
			    int64_I2L(tmp64, LINK_SEVER_SHORT);
			}
			else {
			    int64_I2L(tmp64, LINK_SEVER_LONG);
			}
			int64_MUL(tmp64, tmp64, MILLION64);
			schedule_event(EV_REBOOT, -l-1,	/* neg(linkno - 1) */
					tmp64, NULLTIMER, 0);
			break;
    }
    if(dflag)
	fprintf(stderr,"%s->%s.menu -> %s\n",
			NP[LP[l].endmin].nodename, NP[LP[l].endmax].nodename,
			menu_strings[s]);
    report_linkstate(l);

    return TCL_OK;
}

void display_linkmenu(int l, int x, int y)
{
    static int	first_time	= TRUE;

    if(first_time) {
	int	s;

	sprintf(chararray, "Sever %ds, reconnect", LINK_SEVER_SHORT);
	menu_strings[2]	= strdup(chararray);
	sprintf(chararray, "Sever %ds, reconnect", LINK_SEVER_LONG);
	menu_strings[3]	= strdup(chararray);

	TCLTK_createcommand("linkmenu_select", linkmenu_select);
	for(s=0 ; s<N_LINKMENU_STRINGS ; s++) {
	    sprintf(chararray,"linkmenu_label(%d)", s);
	    Tcl_SetVar(tcl_interp, chararray,
				menu_strings[s], TCL_GLOBAL_ONLY);
	}
	first_time = FALSE;
    }
    TCLTK("PopupLinkMenu %d \"%s->%s\" .%s %d %d %d",
			l,
			NP[LP[l].endmin].nodename, NP[LP[l].endmax].nodename,
			argv0,
			linkmenu_mask(l),
			x, y);
}

#endif	/* defined(USE_TCLTK) */

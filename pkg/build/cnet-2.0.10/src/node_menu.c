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
	"Reboot",
	"Crash",
	"Shutdown, reboot",
	"Pause 10s, resume",
	"Pause 60s, resume"
};

#define	N_NODEMENU_STRINGS	(sizeof(menu_strings) / sizeof(menu_strings[0]))

#define	RE		0
#define	CR		1
#define	SR		2
#define	P1		3
#define	P6		4

#define	REMASK		(1<<RE)
#define	CRMASK		(1<<CR)
#define	SRMASK		(1<<SR)
#define	P1MASK		(1<<P1)
#define	P6MASK		(1<<P6)


static int nodemenu_mask(int n)
{
    int	mask;

    switch ((int)NP[n].runstate) {
    case STATE_PAUSED :
    case STATE_RUNNING :
	mask	= REMASK | CRMASK | SRMASK | P1MASK | P6MASK;	break;
    case STATE_CRASHED :
    case STATE_UNDERREPAIR :
	mask	= REMASK;					break;
    default :
	mask	= 0;						break;
    }
    return(mask);
}

static int nodemenu_select(ClientData data, Tcl_Interp *interp,
				int argc, char *argv[])
{
    extern void	unschedule_node(int node);
    int		n, s;

    if(vflag) {
	for(n=0 ; n<argc-1 ; ++n)
	    fprintf(stderr,"%s ", argv[n]);
	fprintf(stderr,"%s\n", argv[argc-1]);
    }
    if(argc != 3) {
	interp->result	= "wrong # args";
	return TCL_ERROR;
    }
    n	= atoi(argv[1]);
    s	= atoi(argv[2]);
    if(n < 0 || n >= _NNODES || s < 0 || s >= N_NODEMENU_STRINGS) {
	interp->result	= "invalid node or menu arg";
	return TCL_ERROR;
    }

    switch (s) {
    case RE :	NP[n].runstate  = STATE_REBOOTING;
		schedule_event(EV_REBOOT, n, int64_ONE, NULLTIMER,
				NP[n].data[(int)EV_REBOOT]);
		break;
    case CR :	NP[n].runstate  = STATE_CRASHED;
		unschedule_node(n);
		break;
    case SR :	NP[n].runstate	= STATE_AUTOREBOOT;
		schedule_event(EV_REBOOT, n, int64_ONE, NULLTIMER,
				NP[n].data[(int)EV_REBOOT]);
		break;
    case P1 :
    case P6 :	NP[n].runstate	= STATE_PAUSED;
		int64_I2L(NP[n].will_resume, (s == P1 ? 10000000 : 60000000));
		schedule_event(EV_REBOOT, n, NP[n].will_resume, NULLTIMER,
				NP[n].data[(int)EV_REBOOT]);
		int64_ADD(NP[n].will_resume, TIMENOW_in_USEC,NP[n].will_resume);
		break;
    }
    draw_node_icon(FALSE,n);
    if(dflag)
	fprintf(stderr,"%s.menu -> %s\n", NP[n].nodename, menu_strings[s]);

    return TCL_OK;
}

void display_nodemenu(int n)
{
    static int	first_time	= TRUE;

    if(first_time) {
	int	s;

	TCLTK_createcommand("nodemenu_select", nodemenu_select);
	for(s=0 ; s<N_NODEMENU_STRINGS ; s++) {
	    sprintf(chararray,"nodemenu_label(%d)", s);
	    Tcl_SetVar(tcl_interp, chararray,
				menu_strings[s], TCL_GLOBAL_ONLY);
	}
	first_time = FALSE;
    }
    TCLTK("PopupNodeMenu %d %s .%s %d %d %d",
			n, NP[n].nodename,
			argv0,
			nodemenu_mask(n),
			NP[n].nattr.x, NP[n].nattr.y);
}

#endif	/* defined(USE_TCLTK) */

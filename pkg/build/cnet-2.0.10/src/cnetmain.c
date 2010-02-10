#include "cnetheader.h"
#include "statistics.h"

#if	!defined(USE_WIN32)
#include <sys/resource.h>
#endif

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

static void usage(int status)
{
    fprintf(stderr, "Usage: %s [options] {TOPOLOGY | -r nnodes}\n",argv0);
    fprintf(stderr, "options are:\n");

    fprintf(stderr,
    "  %cA str\tprovide the application layer compilation string\n", OPTION_CH);
    fprintf(stderr,
    "  %cc\t\tkeep all node clocks synchronized\n", OPTION_CH);
    fprintf(stderr,
    "  %cC str\tprovide the main protocol compilation string\n", OPTION_CH);
    fprintf(stderr,
    "  %cd\t\tprovide some debug printing\n", OPTION_CH);
    fprintf(stderr,
    "  %cDtoken\tdefine C preprocessor tokens for compilation\n", OPTION_CH);
    fprintf(stderr,
    "  %ce\t\thave receiver report corrupt frame arrival with ER_CORRUPTDATA\n",
				OPTION_CH);
    fprintf(stderr,
    "  %cE n\t\texecute for the indicated number of events\n", OPTION_CH);
    fprintf(stderr,
    "  %cf secs\tset the frequency of reporting statistics (with -s or -z)\n",
				OPTION_CH);
    fprintf(stderr,
    "  %cF filenm\tprovide the Tcl/Tk source filename\n", OPTION_CH);
    fprintf(stderr,
    "  %cg\t\tgo - commence execution immediately\n", OPTION_CH);
    fprintf(stderr,
    "  %cIdirectory\tprovide a C preprocessor include path\n", OPTION_CH);
    fprintf(stderr,
    "  %ck\t\tuse the old K&R C compiler\n", OPTION_CH);
    fprintf(stderr,
    "  %cm mins\trun for the indicated length of wall-clock time\n", OPTION_CH);
    fprintf(stderr,
    "  %cM mins\trun for the indicated length of simulated time\n", OPTION_CH);
    fprintf(stderr,
    "  %cn\t\tcompile and link the protocols, but do not run\n", OPTION_CH);
    fprintf(stderr,
    "  %co name\tprovide the prefix of each node's output file\n", OPTION_CH);
    fprintf(stderr,
    "  %cO\t\topen each node's window on startup\n", OPTION_CH);
    fprintf(stderr,
    "  %cp\t\tprint the topology, as read, and exit\n", OPTION_CH);
    fprintf(stderr,
    "  %cP str\tprovide the physical layer compilation string\n", OPTION_CH);
    fprintf(stderr,
    "  %cq\t\trequest quiet execution, except during EV_DEBUGs\n", OPTION_CH);
    fprintf(stderr,
    "  %cr n\t\trequest a random, connected, topology of n nodes\n", OPTION_CH);
    fprintf(stderr,
    "  %cR func\tname the function to reboot each node\n", OPTION_CH);
    fprintf(stderr,
    "  %cs\t\treport all non-zero execution statistics\n", OPTION_CH);
    fprintf(stderr,
    "  %cS seed\tprovide the starting random seed\n", OPTION_CH);
    fprintf(stderr,
    "  %ct\t\ttrace all events via cnet's stderr stream\n", OPTION_CH);
    fprintf(stderr,
    "  %cT\t\tuse discrete-event simulation\n", OPTION_CH);
    fprintf(stderr,
    "  %cUtoken\tundefine C preprocessor tokens for compilation\n", OPTION_CH);
    fprintf(stderr,
    "  %cv\t\tprint version (%s) and provide verbose debug\n",
			OPTION_CH, CNET_VERSION);
    fprintf(stderr,
    "  %cW\t\tdo not use the windowing interface\n", OPTION_CH);
    fprintf(stderr,
    "  %cz\t\treport all statistics, even if they are zero\n", OPTION_CH);

    vflag	= FALSE;
    fprintf(stderr, "\nThe cnet header file is %s.\n",
			find_cnetfile("cnet.h", FALSE, TRUE));
    fprintf(stderr, "Protocols will be compiled with %s.\n",
#if     USE_GCC_COMPILER
			findenv("CNETGCC", CNETGCC));
#else
			findenv("CNETCC", CNETCC));
#endif

    fprintf(stderr,"Your compiler %s native support for the CnetInt64 type.\n",
#if     HAVE_LONG_LONG || defined(USE_WIN32)
	"provides");
#else
	"does not provide");
#endif

    fprintf(stderr, "Protocols will be linked with %s.\n",
			findenv("CNETLD", CNETLD));

    fprintf(stderr, "\nPlease report bugs to %s\n", CNET_EMAIL);
    exit(status);
}


static	int	mflag	= 3;			/* minutes of real execution */
static	int	Mflag	= 1000000;		/* minutes of simulated time */
static	int	sflag	= FALSE;		/* display statistics/summary */

void cleanup(int status)
{
    if(nerrors)
	fprintf(stderr,"\n%d error%s found.\n",nerrors,nerrors==1?"":"s");

    if(gattr.tfp)				/* close trace-file if open */
	fclose(gattr.tfp);

    if(status == 0) {
	extern void	invoke_shutdown(int node);
	extern void	stats_summary(void);

	int	n;

	for(n=0 ; n<_NNODES ; ++n)
	    invoke_shutdown(n);
	if(sflag)
	    stats_summary();
    }
    exit((status == 0) ? 0 : 1);
}


/* -------------------------- Overtime errors -------------------------- */


static void signal_catcher(int sig)
{
#if	!defined(USE_WIN32)
    if(sig == SIGALRM || sig == SIGXCPU) {
	fprintf(stderr, "%s: %d minute time limit exceeded\n(see %s)\n",
			    argv0, sig == SIGALRM ? mflag : Mflag, WWW_FAQ);
	cleanup(0);
    }
#endif

    fprintf(stderr,"%s: caught signal number %d", argv0, sig);
    if(THISNODE >= 0)			/* only if we've been running */
	fprintf(stderr," while (last) handling %s.%s\n",
			NP[THISNODE].nodename, cnet_evname[(int)HANDLING]);
    else
	fputc('\n',stderr);

    if(sig == SIGINT)
	cleanup(0);

    if(sig == SIGBUS || sig == SIGSEGV)
	fprintf(stderr, "(see %s)\n", WWW_FAQ);

    if((char *)malloc(1024) == (char *)NULL)
	fprintf(stderr,"%s: Out of memory!\n",argv0);
    _exit(2);
}


#if	defined(USE_TCLTK)
static void tcl_times_up(ClientData client_data)
{
#if !defined(USE_WIN32)
    signal_catcher(SIGALRM);
#endif
}
#endif


static void init_traps(int mflag)
{
#if	defined(USE_TCLTK)
    if(Wflag) {
	Tcl_CreateTimerHandler(mflag * 60000 /* yes, in millisecs */,
				(Tcl_TimerProc *)tcl_times_up, (ClientData)0);
    }
    else
#endif

#if	!defined(USE_WIN32)
    {
	signal(SIGBUS,	signal_catcher);
	signal(SIGQUIT,	signal_catcher);
	signal(SIGALRM,	signal_catcher);
	alarm((unsigned int)(mflag*60));
    }
#endif

    signal(SIGINT,	signal_catcher);
    signal(SIGSEGV,	signal_catcher);
    signal(SIGILL,	signal_catcher);
    signal(SIGFPE,	signal_catcher);
}

/* ---------------------------------------------------------------- */


int main(int argc, char **argv)
{
    extern void	init_globals(void);
    extern void	init_application_layer(char *, int, int);
    extern void	init_physical_layer(char *, int, int, int, int);
    extern void	init_stats_layer(int);
    extern void	init_stdio_layer(char *);
    extern void	init_trace(void);
    extern void	init_topology(int, char *, int, int, char *, int, int);
    extern void	init_scheduler(int, int, int, int);

#if	defined(USE_TCLTK)
    extern void	init_mainwindow(char *, int, char *);
    extern void	init_statswindow(void);
    extern void	init_loadwindow(void);
#endif

    extern void check_topology(int Tflag, int argc, char **argv);
    extern void compile_topology(int);
    extern void motd(void);
    extern void parse_topology(FILE *, char *, char **);
    extern void save_topology(char *);
    extern void random_topology(int);
    extern int	schedule(int, int);


    extern int	atoi(const char *);

    char	**cnet_argv;
    int		  cnet_argc;

    FILE	*fp;
    char	*defines[64];		/* hoping that this is enough */
    int		ndefines = 0;

    char	*Aflag	= NULL,		/* application layer file string */
		*Cflag	= NULL,		/* protocol source file string */
		*Fflag	= NULL,		/* TclTk filename, if needed */
		*oflag	= NULL,		/* prefix of output filenames */
		*Pflag	= NULL,		/* physical layer file string */
		*Rflag	= NULL;		/* reboot_func function name */
    char	*topfile= NULL;		/* topology filename */

    int		cflag	= FALSE,	/* synchronize time_of_day clocks */
    		Eflag	= int32_MAXINT,	/* max. number of events to run */
    		fflag	= 0,		/* freq. (secs) of scheduler reports */
    		gflag	= FALSE,	/* start execution (go) immediately */
		Nflag	= FALSE,	/* divulge NNODES */
		nflag	= FALSE,	/* compile, link, exit */
		Oflag	= FALSE,	/* open all node windows */
		pflag	= FALSE,	/* display topology, exit */
		qflag	= FALSE,	/* quiet execution (except EV_DEBUGs) */
		rflag	= 0,		/* r>1 => random topology */
		Sflag	= 0,		/* random seed */
		tflag	= FALSE,	/* trace event handlers */
		Tflag	= FALSE,	/* use fast-as-you-can scheduling */
		zflag	= FALSE;	/* display zero statistics */

    int		eflag	= REPORT_PHYSICAL_CORRUPTION;
    int		kflag	= DEFAULT_KR_COMPILER;

    argv0 = (argv0 = strrchr(argv[0],'/')) ? argv0+1 : argv[0];
    argc--;				/* Process command-line arguments */
    argv++;
    cnet_argv	= argv;
    cnet_argc	= argc;

    Sflag	= (time((time_t *)NULL) << 16) + getpid();

#if	defined(USE_TCLTK) && !defined(USE_WIN32)
    if(findenv("DISPLAY", NULL) != (char *)NULL) {
	Fflag		= findenv("CNETTCLTK", CNETTCLTK);
	tcl_interp	= Tcl_CreateInterp();
	cnet_argv	= argv;
	cnet_argc	= argc;
    }
#endif

    while((cnet_argc > 0) && (**cnet_argv == OPTION_CH)) {
            switch (*(*cnet_argv+1)) {

/* USE MY APPLICATION LAYER */
	    case 'A' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc == 0 || cnet_argv[0][0] == OPTION_CH) {
			    fprintf(stderr,
			    "%s: application layer compile string expected\n",
				argv0);
			    cnet_argc = 0;
			}
			else
			    Aflag = strdup(*cnet_argv);
			break;

/* KEEP CLOCKS SYNOPTION_CHRONIZED */
	    case 'c' :	cflag = !cflag;		break;

/* COMPILE THIS PROTOCOL SOURCE STRING */
	    case 'C' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc == 0 || cnet_argv[0][0] == OPTION_CH) {
			    fprintf(stderr,
			       "%s: compile string expected\n",argv0);
			    cnet_argc = 0;
			}
			else
			    Cflag = strdup(*cnet_argv);
			break;

/* DEBUG PRINTING */
	    case 'd' :	dflag = !dflag;		break;

/* ACCEPT SOME CPP SWITCHES */
	    case 'D' :
	    case 'U' :
	    case 'I' :	defines[ndefines++] = *cnet_argv;
						break;

/* EXECUTE FOR A SPECIFIC NUMBER OF EVENTS */
	    case 'E' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc > 0 && isdigit((int)(*cnet_argv[0]))) {
			    Eflag = atoi(*cnet_argv);
			    break;
			}
			fprintf(stderr,
				"%s : invalid number of events\n",argv0);
			cnet_argc = 0;
			break;

/* REPORT OPTION_CHECKSUM ERRORS WITH ER_CORRUPTDATA */
	    case 'e' :	eflag = !eflag;		break;

/* PROVIDE TCL/TK FILENAME */
	    case 'F' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc == 0 || cnet_argv[0][0] == OPTION_CH) {
			    fprintf(stderr,
			       "%s: Tcl/Tk filename expected\n",argv0);
			    cnet_argc = 0;
			}
			else
			    Fflag = strdup(*cnet_argv);
			break;

/* FREQUENCY OF SOPTION_CHEDULER REPORTING (in seconds) */
	    case 'f' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc > 0 && isdigit((int)(*cnet_argv[0]))) {
			    fflag = atoi(*cnet_argv);
			    break;
			}
			fprintf(stderr,
				"%s : invalid number of seconds\n",argv0);
			cnet_argc = 0;
			break;

/* COMMENCE EXECUTION (go) IMMEDIATELY */
	    case 'g' :	gflag = !gflag;		break;

/* USE "OLD" K&R C COMPILER */
	    case 'k' :	kflag = !kflag;		break;

/* RUN FOR mflag MINUTES OF WALL-CLOCK TIME */
	    case 'm' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc > 0 && (mflag=atoi(*cnet_argv)) > 0)
			    break;
			fprintf(stderr,
				"%s : invalid number of minutes\n",argv0);
			cnet_argc = 0;
			break;

/* RUN FOR Mflag MINUTES OF SIMULATED TIME */
	    case 'M' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc > 0 && (Mflag=atoi(*cnet_argv)) > 0)
			    break;
			fprintf(stderr,
				"%s : invalid number of minutes\n",argv0);
			cnet_argc = 0;
			break;

/* compile and link but DO NOT RUN */
	    case 'n' :	nflag = !nflag;		break;

/* provide NNODES in students' protocols */
	    case 'N' :	Nflag = !Nflag;		break;

/* OUTPUT FILENAME */
	    case 'o' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc == 0 || cnet_argv[0][0] == OPTION_CH) {
			    fprintf(stderr,
			    "%s: output filename prefix expected\n", argv0);
			    cnet_argc = 0;
			}
			else
			    oflag = strdup(*cnet_argv);
			break;
/*  OPEN ALL NODE WINDOWS */
	    case 'O' :	Oflag = !Oflag;		break;

/*  PRINT TOPOLOGY */
	    case 'p' :	pflag = !pflag;		break;

/* USE MY PHYSICAL LAYER */
	    case 'P' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc == 0 || cnet_argv[0][0] == OPTION_CH) {
			    fprintf(stderr,
			    "%s: physical layer compile string expected\n",
				argv0);
			    cnet_argc = 0;
			}
			else
			    Pflag = strdup(*cnet_argv);
			break;

/* quiet execution (except during EV_DEBUGs) */
	    case 'q' :	qflag = !qflag;		break;

/* RANDOM TOPOLOGY */
	    case 'r' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc > 0 && (rflag=atoi(*cnet_argv)) > 1) {
			    --cnet_argc; ++cnet_argv;
			    goto args_ok;
			}
			fprintf(stderr,
			    "%s : invalid number of random nodes\n",argv0);
			cnet_argc = 0; rflag = (-1);
			break;

/* reboot_func() FUNCTION NAME */
	    case 'R' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc == 0 || cnet_argv[0][0] == OPTION_CH) {
			    fprintf(stderr,
			      "%s: EV_REBOOT handler name expected\n",argv0);
			    cnet_argc = 0;
			}
			else
			    Rflag = strdup(*cnet_argv);
			break;

/* SUMMARIZE EXECUTION STATISTICS */
	    case 's' :	sflag = !sflag;		break;

/* PROVIDE RANDOM SEED */
	    case 'S' :	--cnet_argc; ++cnet_argv;
			if(cnet_argc > 0 && (Sflag=atoi(*cnet_argv)) >= 0)
			    break;
			fprintf(stderr,"%s : invalid seed\n",argv0);
			cnet_argc = 0;
			break;
/* TRACE EVENTS */
	    case 't' :	tflag = !tflag;		break;
/* TOGGLE USE OF DISCRETE-EVENT SIMULATION */
	    case 'T' :	Tflag = !Tflag;		break;
/* TOGGLE USE OF THE WINDOWING ENVIRONMENT */
	    case 'X' :
	    case 'W' :	Wflag = FALSE;		break;
/* BE VERY VERBOSE ABOUT cnet's ACTIONS */
	    case 'v' :	vflag = !vflag;		break;
/* DISPLAY STATISTICS EVEN IF ZERO */
	    case 'z' :	zflag = !zflag;		break;

	    default :
		  fprintf(stderr,"%s : illegal option %s\n", argv0,*cnet_argv);
		  cnet_argc = 0;
	    }

	--cnet_argc; ++cnet_argv;
    }
    if(cnet_argc <= 0 && rflag <= 0)
	usage(1);

args_ok:
    if(vflag) {
	dflag	= TRUE;
	fprintf(stderr,"%s\n", CNET_VERSION);
    }

    srand48(Sflag);

#if	defined(USE_TCLTK) && !defined(USE_WIN32)
    if(Wflag && findenv("DISPLAY", NULL) == (char *)NULL) {
	fprintf(stderr,
    "*** warning - DISPLAY variable not defined, using ASCII environment\n");
	Wflag	= FALSE;
    }
#endif

    if(Wflag == FALSE)			/* no windowing => stdio is quiet */
	qflag	= TRUE;

    motd();
    init_globals();
    init_topology(cflag, Cflag, Oflag, qflag, Rflag, Sflag, tflag);
    if(rflag > 1)
	random_topology(rflag);
    else {
	char	*dot;

	if((fp = fopen(*cnet_argv,"r")) == NULL) {
	    fprintf(stderr,"%s : cannot open %s\n",argv0,*cnet_argv);
	    cleanup(1);
	}
	if((dot=strrchr(*cnet_argv,'.')) && strcmp(dot,".c") == 0) {
	    fprintf(stderr,
		"%s: hmmm, '%s' looks like a C file, not a topology file.\n",
				    argv0, *cnet_argv);
	    cleanup(1);
	}
	defines[ndefines]	= (char *)NULL;

	topfile	= strdup(*cnet_argv);
	parse_topology(fp, topfile, defines);
	cnet_argc--; cnet_argv++;
    }

    check_topology(Tflag, cnet_argc, cnet_argv);
    compile_topology(kflag);

    if(pflag) {
	save_topology((char *)NULL);
	exit(EXIT_SUCCESS);
    }
    if(nflag)
	exit(EXIT_SUCCESS);
    if(zflag)
	sflag	= TRUE;

#if	defined(USE_TCLTK)
    if(Wflag) {
	char	winname[32];

	if(rflag)
	    sprintf(winname, "random(%d)", rflag);
	init_mainwindow(Fflag, gflag, rflag ? winname : topfile);
	init_statswindow();
	init_loadwindow();
    }
#endif

    init_application_layer(Aflag, kflag, Sflag);
    init_physical_layer(Pflag, eflag, kflag, Nflag, Sflag);
    init_stats_layer(zflag);
    init_stdio_layer(oflag);
    init_trace();
    init_traps(mflag);
    init_scheduler(fflag, Mflag, Nflag, Sflag);

#if	defined(USE_TCLTK)
    if(Wflag) {
	int		n;

	for(n=0 ; n<_NNODES ; ++n) {
	    extern void	init_nodewindow(int);
	    extern void	toggle_node_window(int);

	    init_nodewindow(n);
	    if(NP[n].nattr.winopen)
		toggle_node_window(n);
	}

	cnet_state = gflag ? STATE_RUNNING : STATE_PAUSED;
	while(Eflag > 0 && cnet_state != STATE_UNKNOWN) {

	    switch ((int)cnet_state) {

	    case STATE_PAUSED:
		tcltk_notify_start();
		break;
	    case STATE_RUNNING:
		Eflag = schedule(Eflag, Tflag);
		break;
	    case STATE_SINGLESTEP: {
		schedule(1, Tflag);
		--Eflag;
		cnet_state = STATE_PAUSED;
		flush_allstats((ClientData)FALSE);
		break;
	    }
	    case STATE_TIMESUP:
#if	!defined(USE_WIN32)
		signal_catcher(SIGALRM);	/* Mflag minutes have expired */
#endif
		break;
	    default :
		cnet_state = STATE_UNKNOWN;
		break;
	    }
	}
    }
    else
#endif	/* USE_TCLTK */
    {
	if(dflag)
	    fprintf(stderr,"running\n");
	schedule(Eflag, Tflag);
    }
    cleanup(0);
    return(EXIT_SUCCESS);
}

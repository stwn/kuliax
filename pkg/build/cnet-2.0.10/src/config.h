
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


/*  Firstly, define your operating system of choice.
    The following preprocessor tests should determine this automatically.
 */

#if	defined(__linux__) && defined(__ELF__)
#define	USE_LINUX

#elif	defined(__osf__) && (defined(__alpha__) || defined(__alpha))
#define	USE_OSF1

#elif	defined(sun) || defined(__sun__)
#if	defined(SVR4) || defined(__svr4__)
#define	USE_SOLARIS
#else
#define	USE_SUNOS
#endif

#elif	defined(sgi) || defined(__sgi) || defined(__sgi__)
#define	USE_IRIX5

#elif  defined(__FreeBSD__)
#define	USE_FREEBSD
 
#elif  defined(__NetBSD__)
#define	USE_NETBSD
 
#elif	defined(_WIN32)
/*  Don't get too excited here!  The Windows implementation is still being
    developed and will first appear in the non-alpha version of 1.9.
 */
#define	USE_WIN32

#else
#error	Could not determine your operating system in config.h
#endif


/*  Next define the following constants to support 64-bit arithmetic
    on this platform.  If your compiler provides 64-bit integers using
    the syntax of 'long long', define HAVE_LONG_LONG as TRUE here,
    else define it to FALSE.

    For GNU's gcc (anywhere) we should have HAVE_LONG_LONG as TRUE.  However,
    note that with gcc and HAVE_LONG_LONG as TRUE you will not be able
    to compile with the -pedantic flag without warnings about 'long long'.

    Also indicate the size of your machine's/compiler's int and
    long variables  (no, we cannot just use 'sizeof(int)' here, sorry).
    .... wish there was a way to do this automatically.
 */

#define	HAVE_LONG_LONG	TRUE
#define	SIZEOF_INT	4
#define	SIZEOF_LONG	4


/*  Next, indicate the type of interface we want.  The only choices are:

	USE_ASCII	or
	USE_TCLTK
 */

#define	USE_TCLTK


/*  Next, define constants such as full pathnames of the C compiler
    components for your selected operating system.
 */

#if	defined(USE_LINUX)
#define	OS_DEFINE	"-DLINUX"
/* no CNETCPP, we will be using gcc -E to perform C preprocessing */
#define	CNETCC		"/usr/bin/gcc"
#define	CNETGCC		"/usr/bin/gcc"
#define	CNETLD		"/usr/bin/gcc"

#elif	defined(USE_SUNOS)
#define	OS_DEFINE	"-DSUNOS"
#define	CNETCPP		"/lib/cpp"
#define	CNETCC		"/bin/cc"
#define	CNETGCC		"/usr/local/bin/gcc"
#define	CNETLD		"/bin/ld"

#elif	defined(USE_SOLARIS)
#define	OS_DEFINE	"-DSOLARIS"
#define	CNETCPP		"/usr/ccs/lib/cpp"
#define	CNETCC		"/usr/ucb/cc"
#define	CNETGCC		"/usr/gnu/bin/gcc"
#define	CNETLD		"/usr/ccs/bin/ld"

#elif	defined(USE_OSF1)
#define	OS_DEFINE	"-DOSF1"
#define	CNETCPP		"/bin/cpp"
#define	CNETCC		"/bin/cc"
#define	CNETGCC		"/usr/local/bin/gcc"
#define	CNETLD		"/bin/ld"

#elif	defined(USE_IRIX5)
#define	OS_DEFINE	"-DIRIX5"
#define	CNETCPP		"/usr/lib/cpp"
#define	CNETCC		"/bin/cc"
#define	CNETGCC		"/usr/local/bin/gcc"
#define	CNETLD		"/bin/ld"

#elif	defined(USE_FREEBSD)
#define	OS_DEFINE	"-DFREEBSD"
#define	CNETCPP		"/usr/bin/cpp"
#define	CNETCC		"/usr/bin/cc"
#define	CNETGCC		"/usr/bin/gcc"
#define	CNETLD		"/usr/bin/ld"

#elif	defined(USE_NETBSD)
#define	OS_DEFINE	"-DNETBSD"
#define	CNETCPP		"/usr/bin/cpp"
#define	CNETCC		"/usr/bin/cc"
#define	CNETGCC		"/usr/bin/gcc"
#define	CNETLD		"/usr/bin/ld"

#elif	defined(USE_WIN32)
#define	OS_DEFINE	"/DWIN32"
#endif


/*  The character constant PATH_SEPARATOR indicates how entries in CNETPATH
    are separated.  Under Unix/Linux this is best ':', but under WIN32 this
    should best be ';'
 */

#if	defined(USE_WIN32)
#define	PATH_SEPARATOR			';'
#else
#define	PATH_SEPARATOR			':'
#endif


/*  The string constant CNETPATH should provide a PATH_SEPARATOR-separated
    list of directory names where cnet will search for <cnet.h> at run-time.
    This value will be the default, but any user can over-ride it by
    having their own $CNETPATH environment variable.
    CNETPATH must be defined.
 */

#if	defined(USE_FREEBSD)
#define	CNETPATH	"/usr/local/share/cnet:/usr/local/include:/usr/local/cnetlib"
#else
#define	CNETPATH	"/usr/share/cnet:/usr/lib/cnet:/cslinux/cnetlib"
#endif


/*  A small number of common errors are detected by cnet at run-time.
    To assist students in quickly finding an explanation for the error,
    we just refer them to cnet's FAQ.
 */

#define	WWW_FAQ		"http://www.csse.uwa.edu.au/cnet/faq.html"


/*  The character constant OPTION_CH defines the character used to introduce
    command-line options.  For Unix/Linux this is best '-', but for WIN32
    you may wish this to be '-' or '/'.
 */

#if	defined(USE_WIN32)
#define	OPTION_CH		'-'
#else
#define	OPTION_CH		'-'
#endif


/*  When compiled for Tcl/Tk, the Tcl/Tk script file may be taken from
    CNETTCLTK or provided with the -F command-line option.
    If an absolute filename, it is sought as is; if a relative filename,
    it is sought in one of the directories in CNETPATH.
 */

#define	CNETTCLTK			"cnet.tcl"


/*  The student protocols may be compiled with either the C compiler
    supplied with your operating system, or with the GNU C compiler, gcc.
    Define USE_GCC_COMPILER to TRUE (1) here if you wish to use gcc.
    Ignored for WIN32.
 */

#define	USE_GCC_COMPILER		TRUE


/*  The student protocols may be compiled with either an "old" K&R compiler
    or an ANSI-C compiler, such as gcc -ansi . If you wish the *default*
    compiler to be K&R, define DEFAULT_KR_COMPILER to FALSE or TRUE here.
    This can be toggled at run-time with the -k option.
    DEFAULT_KR_COMPILER must be defined.
    Ignored for WIN32.
 */

#define	DEFAULT_KR_COMPILER		FALSE


/*  I prefer the rigourous treatment given with gcc -Werror -Wall and
    recommend it as a good habit to adopt. If you're convinced, define
    GCC_WERROR_WANTED and GCC_WALL_WANTED to TRUE here.
    Ignored for WIN32.
 */

#define	GCC_WERROR_WANTED		TRUE
#define	GCC_WALL_WANTED			TRUE


/*  Each node may have a "compile string" provided in the topology file
    or with the -C command-line argument.  This string may include
    C compiler directives and the names of several source files
    containing the protocols. It is typically just set to a single filename.
    If not provided, DEFAULT_COMPILE_STRING is used.
    DEFAULT_COMPILE_STRING must be defined.
 */

#define	DEFAULT_COMPILE_STRING		"protocol.c"


/*  Each node is rebooted by calling a named entry point in its shared
    object.  This may be specified in the topology file or with the -R
    command-line argument.  It is the name of a function which must be
    in the shared object and be externally visible.
    If not provided, DEFAULT_REBOOT_FUNCTION is used.
    DEFAULT_REBOOT_FUNCTION must be defined.
 */

#define	DEFAULT_REBOOT_FUNCTION		"reboot_node"


/*  Some texts (Tanenbaum, 2nd ed.) suggest that frame corruption errors
    should be reported via the Physical Layer - the datalink software
    should not have to detect corruption with checksum algorithms.  If you
    would like the *default* action to be that CNET_read_physical() in
    the receiver returns -1 and sets cnet_errno=ER_CORRUPTDATA for you,
    define REPORT_PHYSICAL_CORRUPTION to FALSE or TRUE here.
    This can be toggled at run-time with the -e option.
    REPORT_PHYSICAL_CORRUPTION must be defined.
 */

#define	REPORT_PHYSICAL_CORRUPTION	FALSE


/*  The standard Physical Layer implementation (in std_physical.c) corrupts
    and loses data frames subject to certain probabilities.  Corruption is
    always implemented by modifying bytes in the data frames, but may also
    truncate (shorten) some frames as well.
 */

#define	MAY_TRUNCATE_FRAMES		FALSE


/*  You may wish to limit the number of frames that may be "pending"
    on each network link. If so, set MAX_PENDING_FRAMES here to some
    smaller value, like 8.  MAX_PENDING_FRAMES must be defined.
 */

#define	MAX_PENDING_FRAMES		1000


/*  Should each Ethernet Network Interface Card (NIC) only be allowed to
    submit one packet at once, resulting in ER_TOOBUSY if there are many
    collisions, or should the NIC be permitted to buffer an (unlimited)
    number of outgoing packets?  ETHERNICS_CAN_BUFFER must be defined.
 */

#define	ETHERNICS_CAN_BUFFER		TRUE


/*  You may like to adjust the frequency with which statistics are
    updated on the load and statistics popup frames (in milliseconds).
    Change STATS_FREQ and UPDATE_TITLE here to reflect this.
    These must be defined.
 */

#define	STATS_FREQ			500		/* in millisecs */
#define	UPDATE_TITLE			"Updated every 500ms"


/*  Selecting the right mouse button over a link displays a menu from
    which that link may be severed or reconnected.  You may provide here
    a short and a long value, in seconds, for which the link will remain
    severed, after which it is reconnected.
 */

#define	LINK_SEVER_SHORT		10		/* both in secs */
#define	LINK_SEVER_LONG			60


/*  The network map can be drawn to highlight heavily used links by drawing
    them fatter than lesser used links. CANVAS_FATLINK specifies the width
    (in pixels) of heavily used links. If you wish all links to simply be
    drawn with all the same width, #define CANVAS_FATLINK to 1.
 */

#define	CANVAS_FATLINK			6


/*  Each node has an internal clock which may or may not be synchronized
    with the internal clocks of other nodes. NODE_CLOCK_SKEW indicates the
    maximum number of microseconds that each node may randomly differ from
    the "average" time. Defining NODE_CLOCK_SKEW to 0 requests that all
    nodes are initialized with the same time.
    Similarly, the -c command-line switch ensures clock synchronization.
 */

#define	NODE_CLOCK_SKEW			600000000	/* for +/- 10 mins */


/*  You may want a message to appear each time a student runs
    cnet (sort of a message of the day for cnet). For example, I
    have previously ensured that students' projects are in the
    correct directory for automatic collection. If you want this,
    define MOTD_WANTED here and add your own code in motd.c:motd()
 */

#define	MOTD_WANTED			FALSE


/*  Random topologies generated with the -r command-line option are
    placed on a rectangular grid.  Define RANDOM_DIAGONALS to be TRUE
    if you wish diagonal links on this grid.
 */

#define	RANDOM_DIAGONALS		TRUE


/*  I get annoyed if students spell 'receive' as 'recieve'.
    If you would like compile.c to check for this, define
    CHECK_RECEIVE_SPELLING here.
 */

#define	CHECK_RECEIVE_SPELLING		TRUE


/*  There is a very real chance that students' protocols may choose
    variable and function names the same as those that are globally
    accessible within cnet, and I'm not sure how this would affect the
    dynamic linking on (future) implementations.  If you would like global
    names hidden (via hidenames.h), define HIDE_GLOBAL_NAMES to TRUE here.
 */

#define	HIDE_GLOBAL_NAMES		TRUE


/*  DRAWFRAME_WIDTH defines the width of the additional Tcl/Tk canvas
    on which we draw each physical layer frame.
 */

#define	DRAWFRAME_WIDTH			600

#define	COLOUR_ETHERNET			"blue"
#define	COLOUR_POINT2POINT		"magenta"
#define	COLOUR_SEVERED			"red"


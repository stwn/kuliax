#include "cnetheader.h"
#include "lexical.h"

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

TOKEN	token;

static struct {
    char	*word;
    TOKEN	token;
} reserved[] = {

	{ "Bps",		T_BYTESPS			},
	{ "KB",			T_KBYTES			},
	{ "KBps",		T_KBYTESPS			},
	{ "KByte",		T_KBYTES			},
	{ "KBytes",		T_KBYTES			},
	{ "Kbps",		T_KBITSPS			},
	{ "MB",			T_MBYTES			},
	{ "MBbyte",		T_MBYTES			},
	{ "MBbytes",		T_MBYTES			},
	{ "MBps",		T_MBYTESPS			},
	{ "Mbps",		T_MBITSPS			},
	{ "address",		T_ATTR_NODE_ADDRESS		},
	{ "args",		T_ATTR_NODE_REBOOTARGS		},
	{ "argv",		T_ATTR_NODE_REBOOTARGS		},
	{ "bandwidth",		T_ATTR_LINK_BANDWIDTH		},
	{ "bgimage",		T_BGIMAGE			},
	{ "bps",		T_BITSPS			},
	{ "bytes",		T_BYTES				},
	{ "c",			T_C				},
	{ "compile",		T_ATTR_NODE_COMPILE		},
	{ "costperbyte",	T_ATTR_LINK_COSTPERBYTE		},
	{ "costperframe",	T_ATTR_LINK_COSTPERFRAME	},
	{ "delay",		T_ATTR_LINK_PROPAGATIONDELAY	},
	{ "drawframes",		T_DRAWFRAMES			},
	{ "east",		T_EAST,				},
	{ "ethernet",		T_ETHERNET,			},
	{ "false",		T_FALSE				},
	{ "from",		T_FROM				},
	{ "host",		T_HOST				},
	{ "hosttype",		T_HOSTTYPE			},
	{ "link",		T_LINK				},
	{ "linkmtbf",		T_ATTR_LINK_MTBF		},
	{ "linkmttr",		T_ATTR_LINK_MTTR		},
	{ "linktype",		T_LINKTYPE			},
	{ "maxmessagesize",	T_ATTR_NODE_MAXMESSAGESIZE	},
	{ "messagerate",	T_ATTR_NODE_MESSAGERATE		},
	{ "minmessagesize",	T_ATTR_NODE_MINMESSAGESIZE	},
	{ "ms",			T_MSEC				},
	{ "msec",		T_MSEC				},
	{ "msecs",		T_MSEC				},
	{ "nicaddr",		T_NICADDR			},
	{ "node",		T_HOST				},
	{ "nodemtbf",		T_ATTR_NODE_MTBF		},
	{ "nodemttr",		T_ATTR_NODE_MTTR		},
	{ "north",		T_NORTH,			},
	{ "northeast",		T_NORTHEAST,			},
	{ "northwest",		T_NORTHWEST			},
	{ "of",			T_OF				},
	{ "os",			T_ATTR_NODE_OSNAME		},
	{ "osname",		T_ATTR_NODE_OSNAME		},
	{ "ostype",		T_ATTR_NODE_OSNAME		},
	{ "outputfile",		T_ATTR_NODE_OUTPUTFILE		},
	{ "probframecorrupt",	T_ATTR_LINK_PROBFRAMECORRUPT	},
	{ "probframeloss",	T_ATTR_LINK_PROBFRAMELOSS	},
	{ "propagationdelay",	T_ATTR_LINK_PROPAGATIONDELAY	},
	{ "rebootargs",		T_ATTR_NODE_REBOOTARGS		},
	{ "rebootfunc",		T_ATTR_NODE_REBOOTFUNC		},
	{ "router",		T_ROUTER			},
	{ "routertype",		T_ROUTERTYPE			},
	{ "s",			T_SEC				},
	{ "sec",		T_SEC				},
	{ "secs",		T_SEC				},
	{ "showcostperbyte",	T_SHOWCOSTPERBYTE		},
	{ "showcostperframe",	T_SHOWCOSTPERFRAME		},
	{ "sourcefile",		T_ATTR_NODE_COMPILE		},
	{ "sourcefiles",	T_ATTR_NODE_COMPILE		},
	{ "south",		T_SOUTH,			},
	{ "southeast",		T_SOUTHEAST,			},
	{ "southwest",		T_SOUTHWEST,			},
	{ "to",			T_TO				},
	{ "toggle",		T_TOGGLE			},
	{ "trace",		T_ATTR_NODE_TRACE		},
	{ "tracefile",		T_TRACEFILE			},
	{ "transmitbufsize",	T_ATTR_LINK_TRANSMITBUFSIZE	},
	{ "true",		T_TRUE				},
	{ "us",			T_USEC				},
	{ "usec",		T_USEC				},
	{ "usecs",		T_USEC				},
	{ "west",		T_WEST,				},
	{ "winopen",		T_ATTR_NODE_WINOPEN		},
	{ "winx",		T_ATTR_NODE_WINX		},
	{ "winy",		T_ATTR_NODE_WINY		},
	{ "x",			T_ATTR_NODE_X			},
	{ "y",			T_ATTR_NODE_Y			}
};

#define NRESWORDS	(sizeof(reserved) / sizeof(reserved[0]))

int	nextch;

static void cpp_line()
{
    char *p = input.line+1,
	 *q = chararray;

	while(*p && !(isdigit((int)*p)))
	    p++;
	if(!isdigit((int)*p))
	    return;

	input.lc = 0;
	while(*p && isdigit((int)*p))
	    input.lc = input.lc*10 + (*p++ -'0');
	--input.lc;

	while(*p && *p != '"') p++;
	p++;
	while(*p && *p != '"') *q++ = *p++;
	*q = '\0';
	input.name = *chararray ? strdup(chararray) : "<stdin>";
}


static void get()		/* get the next buffered char from input.line */
{
    if(input.cc == input.ll) {
	input.cc = input.ll = 0;
	*input.line = '\0';
	while(fgets(input.line,BUFSIZ,input.fp) != NULL) {
	    char *p = input.line;

	    while(*p++ &= 0177);		/* cleanup line */
	    input.ll = --p - input.line;

	    if(*input.line == '#') {
		cpp_line(); continue;
	    }
	    break;
	}
	input.lc++;
    }
    nextch = input.line[input.cc++];
}

#define unget()		(nextch = input.line[--input.cc])

static void skip_blanks()		/*  Skip spaces, tabs and C-comments */
{
    while(nextch == ' ' || nextch == '\t' || nextch == '\n' || nextch == '/') {
	if(nextch == '/') {
	    get();
	    if(nextch == '/') {		/* A C++ style comment */
		input.cc = input.ll = 0;
		get();
		continue;
	    }
	    if(nextch != '*') {
		unget();
		break;
	    }
	    get();
	    while(!feof(input.fp)) {
		if(nextch == '*') {
		    get();
		    if(nextch == '/') {
			get();
			break;
		    }
		}
	        get();
	    }
	    continue;
	}
	get();
    }
}

static	char *cp;

static void escape_str()
{
    get();
    switch(nextch) {
	case 'b' :
	case 'f' :
	case 'n' :
	case 'r' :
	case 't' :  *cp++ = '\\';
		    *cp++ = nextch;
		    break;
    }
}

static void parse_etheraddr()
{
    while(isxdigit(nextch) || nextch == ':') {
	*cp++	= nextch;
	get();
    }
    unget();
    *cp		= '\0';
    token	= (CNET_parse_nicaddr(input.nicaddr, chararray) == 0)
			    ? T_ETHERADDR : T_BAD;
}

void gettoken()
{
    int	pos;

    get();
    skip_blanks();

    *chararray	= '\0';
    input.value	= 0;
    if(isalpha(nextch)) {
	cp = chararray;
	while(isalnum(nextch) || nextch =='_'|| nextch=='-' || nextch=='.') {
	    *cp++ = nextch;
	    get();
	}
	*cp = '\0';
	unget();
	for(pos=0; pos < NRESWORDS ; pos++) {		/*  reserved word?  */
	    int i;

	    if((i=strcmp(chararray,reserved[pos].word))==0) {
		token = reserved[pos].token;
		return;
	    }
	    else if(i<0)
		break;
	}
	if(islower((int)*chararray)) {
#if !defined(toupper)
	    extern int	toupper(int);
#endif

	    *chararray = toupper((int)*chararray);
	}
	token = T_NAME;
	return;
    }
    else if(isdigit(nextch)) {	/* Collect digits of integer, real or char */
	cp		= chararray;
	input.value	= 0;
	while(isdigit(nextch)) {
	    input.value = input.value*10 + (nextch-'0');
	    *cp++ = nextch;
	    get();
	}
	*cp	= '\0';
	if(nextch == ':') {
	    parse_etheraddr();
	    return;
	}
	if(nextch != '.') {
	    if(nextch == 'k' || nextch == 'K')
		input.value *= K;
	    else
		unget();
	    token = T_INTCONST;
	    return;
	}
	*cp++ = '.';
	get();
	while(isdigit(nextch)) {
	    *cp++ = nextch;
	    get();
	}
	if(nextch == 'E') {
	    *cp++ = 'E';
	    if(nextch == '-' || nextch == '+') {
		*cp++	= nextch;
		get();
	    }
	    while(isdigit(nextch)) {
	        *cp++ = nextch;
	        get();
	    }
	}
	*cp	= '\0';
	unget();
	token	= T_REALCONST;
	return;
    }
    else if(isxdigit(nextch)) {
	cp	= chararray;
	parse_etheraddr();
	return;
    }
    else if(nextch == '"') {
	get();
	cp = chararray;
	while(nextch != '"' && !feof(input.fp)) {
	    if(nextch == '\\')
		escape_str();
	    else
		*cp++ = nextch;
	    get();
	}
	*cp	= '\0';
	token	= feof(input.fp) ? T_BAD : T_STRCONST;

    } else
    switch (nextch) {
	case '{' :	token = T_LCURLY;		break;
	case '}' :	token = T_RCURLY;		break;
	case '=' :	token = T_EQ;			break;
	case ',' :	token = T_COMMA;		break;
	default  :	token = feof(input.fp) ? T_EOF : T_BAD;
							break;
    }
}

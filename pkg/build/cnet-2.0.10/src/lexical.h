
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

typedef enum {

T_EOF		= -1,
T_BAD		=  0,

T_ATTR_LINK_FIRST,
T_ATTR_LINK_BANDWIDTH,
T_ATTR_LINK_COSTPERBYTE,
T_ATTR_LINK_COSTPERFRAME,
T_ATTR_LINK_MTBF,
T_ATTR_LINK_MTTR,
T_ATTR_LINK_PROBFRAMECORRUPT,
T_ATTR_LINK_PROBFRAMELOSS,
T_ATTR_LINK_PROPAGATIONDELAY,
T_ATTR_LINK_TRANSMITBUFSIZE,
T_ATTR_LINK_LAST,

T_ATTR_NODE_FIRST,
T_ATTR_NODE_ADDRESS,
T_ATTR_NODE_COMPILE,
T_ATTR_NODE_MAXMESSAGESIZE,
T_ATTR_NODE_MESSAGERATE,
T_ATTR_NODE_MINMESSAGESIZE,
T_ATTR_NODE_MTBF,
T_ATTR_NODE_MTTR,
T_ATTR_NODE_OSNAME,
T_ATTR_NODE_OUTPUTFILE,
T_ATTR_NODE_REBOOTARGS,
T_ATTR_NODE_REBOOTFUNC,
T_ATTR_NODE_TRACE,
T_ATTR_NODE_WINOPEN,
T_ATTR_NODE_WINX,
T_ATTR_NODE_WINY,
T_ATTR_NODE_X,
T_ATTR_NODE_Y,
T_ATTR_NODE_LAST,

T_BITSPS,
T_BGIMAGE,
T_BYTES,
T_BYTESPS,
T_C,
T_COMMA,
T_DRAWFRAMES,
T_EQ,
T_ETHERADDR,
T_ETHERNET,
T_FALSE,
T_FROM,
T_HOST,
T_HOSTTYPE,
T_INTCONST,
T_KBITSPS,
T_KBYTES,
T_KBYTESPS,
T_LCURLY,
T_LINK,
T_LINKTYPE,
T_MBITSPS,
T_MBYTES,
T_MBYTESPS,
T_MSEC,
T_NAME,
T_NICADDR,
T_OF,
T_RCURLY,
T_REALCONST,
T_ROUTER,
T_ROUTERTYPE,
T_SEC,
T_SHOWCOSTPERBYTE,
T_SHOWCOSTPERFRAME,
T_STRCONST,
T_TO,
T_TOGGLE,
T_TRACEFILE,
T_TRUE,
T_USEC,

T_NORTH,
T_NORTHEAST,
T_EAST,
T_SOUTHEAST,
T_SOUTH,
T_SOUTHWEST,
T_WEST,
T_NORTHWEST

} TOKEN;

extern	TOKEN		token;

extern	void		gettoken(void);

#define	is_node_attr(t)		((int)t>(int)T_ATTR_NODE_FIRST && \
				 (int)t<(int)T_ATTR_NODE_LAST)
#define	is_link_attr(t)		((int)t>(int)T_ATTR_LINK_FIRST && \
				 (int)t<(int)T_ATTR_LINK_LAST)
#define	is_compass_direction(t)	((int)t>=(int)T_NORTH && \
				 (int)t<=(int)T_NORTHWEST)

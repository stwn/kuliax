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


/* ----------------------- CCITT-16 Checksum ---------------------------- */

/*  From comp.lang.c Wed Nov  7 16:07:00 1990
    From: ron@scocan.sco.COM (Ron Irvine)
    Date: 5 Nov 90 22:31:57 GMT
    Originator: ron@capri

    There are two popular 16 bit crc calculations.  The one I
    used is the crc16 used for DDCMP and Bisync communications.
    Its polynomial is: x^16 + x^15 + x^2 + 1, with an initial
    value of 0. I call it crc16().  The other common crc is used
    in ADCCP, HDLC and SDLC; its polynomial is: x^16 + x^12 + x^5 + 1,
    with an initial value of -1.  I call it ccitt(). If you are
    using the crc to hash you may want to play with both or invent
    your own.
 */

/*	Computing a POLY number from the crc equation.
 *	Crc's are usually expressed as an polynomial expression such as:
 *
 *		x^16 + x^12 + x^5 + 1
 *
 *	Since the highest order term is to the power 16 this is a
 *	16 bit crc.  The POLY number is determined by setting bits
 *	corresponding to the power terms in the polynomial equation
 *	in an integer.  To do this we number the bits of the integer
 *	with the most significant bit = bit 0. CAUTION: this is the
 *	opposite to the some bit numbering schemes.  This is due to
 *	the least significant bit first convention of crc calculations.
 *	The above equation becomes:
 *
 *	  1   0   0   0   0   1   0   0   0   0   0   0   1   0   0   0
 *	|___.___.___.___|___.___.___.___|___.___.___.___|___.___.___.___|
 *	  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
 *	msb                                                          lsb
 *
 *	Note: the "1" term is equivalent to "x^0" and the "x^16"
 *	term is ignored (it does determine the length to be 16 bits).
 *	Thus the POLY number is 0x8408 (ccitt crc).
 */

/* CCITT - ADCCP, HDLC, SDLC (16 bit)
 *	x^16 + x^12 + x^5 + 1
 */

#define	INIT_CCITT	-1


/* nibble table for CCITT crc */
static unsigned int ccitt_h[] = {
	0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
	0x8408, 0x9489, 0xa50a, 0xb58b, 0xc60c, 0xd68d, 0xe70e, 0xf78f, };

static unsigned int ccitt_l[] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, };

unsigned short checksum_ccitt(unsigned char *addr, int nbytes)
{
    register unsigned int	n, crc;

    crc = INIT_CCITT;
    while (nbytes-- > 0) {
	n = *addr++ ^ crc;
	crc = ccitt_l[n&0x0f] ^ ccitt_h[(n>>4)&0x0f] ^ (crc>>8);
    };
    return((unsigned short)crc);
}

/* ----------------------- CRC-16 Checksum ---------------------------- */

/*
    From comp.lang.c Wed Nov  7 16:07:00 1990
    From: ron@scocan.sco.COM (Ron Irvine)
    Date: 5 Nov 90 22:31:57 GMT
    Originator: ron@capri

		crc16 - DDCMP and Bisync (16 bit)
			x^16 + x^15 + x^2 + 1
 */

#define	POLY_CRC16	0xa001
#define	INIT_CRC16	0

unsigned short checksum_crc16(unsigned char *addr, int nbytes)
{
    register unsigned short crc, bit;
    register int i;

    crc = INIT_CRC16;
    while(nbytes-- > 0) {
	for (i=1; i <= 0x80; i<<=1) {
	    bit = (((*addr)&i)?0x8000:0);
	    if (crc & 1) bit ^= 0x8000;
	    crc >>= 1;
	    if (bit) crc ^= (int)(POLY_CRC16);
	}
	++addr;
    }
    return crc;
}


/* ----------------------- CRC-32 Checksum ---------------------------- */

/*  CRC-32b version 1.03 by Craig Bruce, 27-Jan-94
**
**  Based on "File Verification Using CRC" by Mark R. Nelson in Dr. Dobb's
**  Journal, May 1992, pp. 64-67.  This program DOES generate the same CRC
**  values as ZMODEM and PKZIP
*/

static unsigned long	table_crc32[256];

#define	INIT_CRC32	0xFFFFFFFF
#define	POLY32_INIT	0xEDB88320L

unsigned long checksum_crc32(unsigned char *addr, int nbytes)
{
    register unsigned	long	crc;
    static		int	first	= TRUE;
    int				c;

    if(first) {			/* generate CRC32 table first time only */
	int	i, j;

	for(i=0; i<256; i++) {
	    crc = i;
	    for(j=8; j>0; j--) {
		if (crc&1) {
		    crc = (crc >> 1) ^ POLY32_INIT;
		} else {
		    crc >>= 1;
		}
	    }
	    table_crc32[i] = crc;
	}
	first	= FALSE;
    }
    crc = INIT_CRC32;
    while(nbytes-- > 0) {
	c	= *addr++ ;
	crc = ((crc>>8) & 0x00FFFFFF) ^ table_crc32[ (crc^c) & 0xFF ];
    }
    return( crc^0xFFFFFFFF );
}


/* ----------------------- INTERNET Checksum ---------------------------- */

/* Calculate the Internet Protocol family checksum algorithm.
   This code is taken from Steven's "Unix Network Programming" pp454-455.
   The algorithm is simple, using a 32-bit accumulator (sum),
   we add sequential 16-bit words to it, and at the end, fold back
   all the carry bits from the top 16 bits into the lower 16 bits.
 */

int checksum_internet(unsigned short *addr, int nbytes)
{
    long                sum;
    unsigned short      oddbyte, answer;

    sum = 0L;
    while(nbytes > 1) {
        sum += *addr++;
        nbytes -= 2;
    }
    if(nbytes == 1) {   /* mop up an odd byte if necessary */
        oddbyte = 0;    /* make sure that the top byte is zero */
        *((unsigned char *)&oddbyte) = *(unsigned char *)addr; /* 1 byte only */
        sum += oddbyte;
    }
    /* Now add back carry outs from top 16 bits to lower 16 bits */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi-16 to lo-16 */
    sum += (sum >> 16);                 /* add carry bits */
    answer  = (unsigned short)~sum;     /* one's complement, then truncate to 16 bits */
    return(answer);
}

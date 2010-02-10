#if     !defined(_CNET64BITS_H)
#define _CNET64BITS_H


#if	!defined(HAVE_LONG_LONG)
#define	HAVE_LONG_LONG		0
#endif

#if	!defined(SIZEOF_INT)
#define	SIZEOF_INT		4
#endif

#if	!defined(SIZEOF_LONG)
#define	SIZEOF_LONG		4
#endif

/* ------------------------------------------------------------------------ */

/*
 * Copyright (c) 1983, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *	may be used to endorse or promote products derived from this software
 *	without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * --Copyright--
 */

#if	defined(vax) ||		\
	defined(ns32000) || 	\
	defined(sun386) || 	\
	defined(i386) ||	\
	defined(__i386__) ||	\
	defined(MIPSEL) || 	\
	defined(_MIPSEL) || 	\
	defined(__alpha__) || 	\
	defined(__alpha) ||	\
	defined(BIT_ZERO_ON_RIGHT)
#define IS_LITTLE_ENDIAN	1
#endif

#if	defined(__BIG_ENDIAN__) ||	\
	defined(sel) ||		\
	defined(pyr) ||		\
	defined(mc68000) ||	\
	defined(sparc) ||	\
	defined(__sparc__) ||	\
	defined(is68k) ||	\
	defined(tahoe) ||	\
	defined(ibm032) ||	\
	defined(ibm370) ||	\
	defined(MIPSEB) ||	\
	defined(_MIPSEB) ||	\
	defined(_IBMR2) ||	\
	defined(apollo) ||	\
	defined(hp9000) ||	\
	defined(hp9000s300) ||	\
	defined(hp9000s800) ||	\
	defined(__ppc__) ||		\
	defined (BIT_ZERO_ON_LEFT)
#define IS_BIG_ENDIAN		1
#endif

#if	!defined(IS_LITTLE_ENDIAN) && !defined(IS_BIG_ENDIAN)	\
	/* you must determine what the correct byte order is for
	 * your compiler - the next line is an intentional error
	 * which will force your compiles to bomb until you fix
	 * the above macros.
	 */
	#error "Unable to determine byte order for your architecture";
#endif


/* ------------------------------------------------------------------------ */

/* Parts from http://lxr.mozilla.org/seamonkey/source/nsprpub/pr/include/ */

/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */


/************************************************************************
** TYPES:	CnetUint32
**		CnetInt32
** DESCRIPTION:
**  The int32 types are known to be 32 bits each. 

************************************************************************/
#if SIZEOF_INT == 4
typedef unsigned int	CnetUint32;
typedef int		CnetInt32;
#define PR_INT32(x)	x
#define PR_UINT32(x)	((unsigned int)x)

#elif SIZEOF_LONG == 4
typedef unsigned long	CnetUint32;
typedef long		CnetInt32;
#define PR_INT32(x)	((long)x)
#define PR_UINT32(x)	((unsigned long)x)
#else
#error No suitable type for CnetInt32/CnetUint32
#endif

/************************************************************************
** TYPES:	CnetUint64
**		CnetInt64
** DESCRIPTION:
**  The int64 types are known to be 64 bits each. Care must be used when
**  declaring variables of type CnetUint64 or CnetInt64. Different hardware
**  architectures and even different compilers have varying support for
**  64 bit values. The only guaranteed portability requires the use of
**  the int64_ macros.

************************************************************************/

#if	HAVE_LONG_LONG
#if SIZEOF_LONG == 8
typedef long		CnetInt64;
typedef unsigned long	CnetUint64;
#elif defined(WIN16)
typedef __int64		CnetInt64;
typedef unsigned __int64 CnetUint64;
#elif defined(WIN32) && !defined(__GNUC__)
typedef __int64		CnetInt64;
typedef unsigned __int64 CnetUint64;
#else
typedef long long	CnetInt64;
typedef unsigned long long CnetUint64;
#endif /* SIZEOF_LONG == 8 */

#else  /* !HAVE_LONG_LONG */
typedef struct {
#ifdef IS_LITTLE_ENDIAN
    CnetUint32 lo, hi;
#else
    CnetUint32 hi, lo;
#endif
} CnetInt64;
typedef CnetInt64 CnetUint64;
#endif /* !HAVE_LONG_LONG */

/************************************************************************
** TYPES:	CnetFloat64
** DESCRIPTION:
**  NSPR's floating point type is always 64 bits. 
************************************************************************/
typedef double	CnetFloat64;

/***********************************************************************
** MACROS:	PR_BIT
**		PR_BITMASK
** DESCRIPTION:
** Bit masking macros.  XXX n must be <= 31 to be portable
***********************************************************************/
#define PR_BIT(n)	((CnetUint32)1 << (n))
#define PR_BITMASK(n)	(PR_BIT(n) - 1)


extern	char *		int64_L2A(CnetInt64 num, int commas);
extern	CnetInt64	int64_A2L(char *str);

extern	CnetInt64	int64_MaxInt(void);
extern	CnetInt64	int64_MinInt(void);

#define int64_MAXINT	int64_MaxInt()
#define int64_MININT	int64_MinInt()

#if	HAVE_LONG_LONG
#define int64_ZERO	0
#define int64_ONE	1

#else
extern	CnetInt64	int64_Zero(void);
extern	CnetInt64	int64_One(void);

#define int64_ZERO	int64_Zero()
#define int64_ONE	int64_One()
#endif


#if	HAVE_LONG_LONG
#if SIZEOF_LONG == 8
#define int64_INIT(hi, lo)  ((((long)hi) << 32) + (long)lo)
#elif (defined(WIN32) || defined(WIN16)) && !defined(__GNUC__)
#define int64_INIT(hi, lo)  ((((i64)hi) << 32) + (i64)lo)
#else
#define int64_INIT(hi, lo)  ((((long long)hi) << 32) + (long long)lo)
#endif


/***********************************************************************
** MACROS:	int64_<relational operators>
**
**  int64_IS_ZERO	Test for zero
**  int64_EQ		Test for equality
**  int64_NE		Test for inequality
**  int64_GE_ZERO	Test for zero or positive
**  int64_CMP		Compare two values
***********************************************************************/
#define int64_IS_ZERO(a)	((a) == 0)
#define int64_EQ(a, b)		((a) == (b))
#define int64_NE(a, b)		((a) != (b))
#define int64_GE_ZERO(a)	((a) >= 0)
#define int64_CMP(a, op, b)	((CnetInt64)(a) op (CnetInt64)(b))
#define int64_UCMP(a, op, b)	((CnetUint64)(a) op (CnetUint64)(b))

/***********************************************************************
** MACROS:	int64_<logical operators>
**
**  int64_AND	Logical and
**  int64_OR	Logical or
**  int64_XOR	Logical exclusion
**  int64_OR2	A disgusting deviation
**  int64_NOT	Negation (one's complement)
***********************************************************************/
#define int64_AND(r, a, b)	((r) = (a) & (b))
#define int64_OR(r, a, b)	((r) = (a) | (b))
#define int64_XOR(r, a, b)	((r) = (a) ^ (b))
#define int64_OR2(r, a)		((r) = (r) | (a))
#define int64_NOT(r, a)		((r) = ~(a))

/***********************************************************************
** MACROS:	int64_<mathematical operators>
**
**  int64_NEG		Negation (two's complement)
**  int64_ADD		Summation (two's complement)
**  int64_SUB		Difference (two's complement)
***********************************************************************/
#define int64_NEG(r, a)		((r) = -(a))
#define int64_ADD(r, a, b)	((r) = (a) + (b))
#define int64_SUB(r, a, b)	((r) = (a) - (b))

/***********************************************************************
** MACROS:	int64_<mathematical operators>
**
**  int64_MUL		Product (two's complement)
**  int64_DIV		Quotient (two's complement)
**  int64_MOD		Modulus (two's complement)
***********************************************************************/
#define int64_MUL(r, a, b)	((r) = (a) * (b))
#define int64_DIV(r, a, b)	((r) = (a) / (b))
#define int64_MOD(r, a, b)	((r) = (a) % (b))

/***********************************************************************
** MACROS:	int64_<shifting operators>
**
**  int64_SHL		Shift left [0..64] bits
**  int64_SHR		Shift right [0..64] bits with sign extension
**  int64_USHR		Unsigned shift right [0..64] bits
**  int64_ISHL		Signed shift left [0..64] bits
***********************************************************************/
#define int64_SHL(r, a, b)	((r) = (CnetInt64)(a) << (b))
#define int64_SHR(r, a, b)	((r) = (CnetInt64)(a) >> (b))
#define int64_USHR(r, a, b)	((r) = (CnetUint64)(a) >> (b))
#define int64_ISHL(r, a, b)	((r) = (CnetInt64)(a) << (b))

/***********************************************************************
** MACROS:	int64_<conversion operators>
**
**  int64_L2I		Convert to signed 32 bit
**  int64_L2UI		Convert to unsigned 32 bit
**  int64_L2F		Convert to floating point
**  int64_L2D		Convert to floating point
**  int64_I2L		Convert signed to 64 bit
**  int64_UI2L		Convert unsigned to 64 bit
**  int64_F2L		Convert float to 64 bit
**  int64_D2L		Convert float to 64 bit
***********************************************************************/
#define int64_L2I(i, l)		((i) = (CnetInt32)(l))
#define int64_L2UI(ui, l)	((ui) = (CnetUint32)(l))
#define int64_L2F(f, l)		((f) = (CnetFloat64)(l))
#define int64_L2D(d, l)		((d) = (CnetFloat64)(l))

#define int64_I2L(l, i)		((l) = (CnetInt64)(i))
#define int64_UI2L(l, ui)	((l) = (CnetInt64)(ui))
#define int64_F2L(l, f)		((l) = (CnetInt64)(f))
#define int64_D2L(l, d)		((l) = (CnetInt64)(d))

/***********************************************************************
** MACROS:	int64_UDIVMOD
** DESCRIPTION:
**  Produce both a quotient and a remainder given an unsigned 
** INPUTS:	CnetUint64 a: The dividend of the operation
**		CnetUint64 b: The quotient of the operation
** OUTPUTS:	CnetUint64 *qp: pointer to quotient
**		CnetUint64 *rp: pointer to remainder
***********************************************************************/
#define int64_UDIVMOD(qp, rp, a, b) \
	(*(qp) = ((CnetUint64)(a) / (b)), \
	 *(rp) = ((CnetUint64)(a) % (b)))

#else  /* !HAVE_LONG_LONG */

#ifdef IS_LITTLE_ENDIAN
#define int64_INIT(hi, lo) {PR_INT32(lo), PR_INT32(hi)}
#else
#define int64_INIT(hi, lo) {PR_INT32(hi), PR_INT32(lo)}
#endif

#define int64_IS_ZERO(a)	(((a).hi == 0) && ((a).lo == 0))
#define int64_EQ(a, b)		(((a).hi == (b).hi) && ((a).lo == (b).lo))
#define int64_NE(a, b)		(((a).hi != (b).hi) || ((a).lo != (b).lo))
#define int64_GE_ZERO(a)	(((a).hi >> 31) == 0)

#define int64_CMP(a, op, b)	(((a).hi == (b).hi) ? ((a).lo op (b).lo) : \
		((CnetInt32)(a).hi op (CnetInt32)(b).hi))
#define int64_UCMP(a, op, b)	(((a).hi == (b).hi) ? ((a).lo op (b).lo) : \
		((a).hi op (b).hi))

#define int64_AND(r, a, b)	((r).lo = (a).lo & (b).lo, \
		(r).hi = (a).hi & (b).hi)
#define int64_OR(r, a, b)	((r).lo = (a).lo | (b).lo, \
		(r).hi = (a).hi | (b).hi)
#define int64_XOR(r, a, b)	((r).lo = (a).lo ^ (b).lo, \
		(r).hi = (a).hi ^ (b).hi)
#define int64_OR2(r, a)		((r).lo = (r).lo | (a).lo, \
		(r).hi = (r).hi | (a).hi)
#define int64_NOT(r, a)		((r).lo = ~(a).lo, \
		(r).hi = ~(a).hi)

#define int64_NEG(r, a)		((r).lo = -(CnetInt32)(a).lo, \
		(r).hi = -(CnetInt32)(a).hi - ((r).lo != 0))
#define int64_ADD(r, a, b) { \
	CnetInt64 _a, _b; \
	_a = a; _b = b; \
	(r).lo = _a.lo + _b.lo; \
	(r).hi = _a.hi + _b.hi + ((r).lo < _b.lo); \
}

#define int64_SUB(r, a, b) { \
	CnetInt64 _a, _b; \
	_a = a; _b = b; \
	(r).lo = _a.lo - _b.lo; \
	(r).hi = _a.hi - _b.hi - (_a.lo < _b.lo); \
}

#define int64_MUL(r, a, b) { \
	CnetInt64 _a, _b; \
	_a = a; _b = b; \
	int64_MUL32(r, _a.lo, _b.lo); \
	(r).hi += _a.hi * _b.lo + _a.lo * _b.hi; \
}

#define _lo16(a)	((a) & PR_BITMASK(16))
#define _hi16(a)	((a) >> 16)

#define int64_MUL32(r, a, b) { \
	CnetUint32 _a1, _a0, _b1, _b0, _y0, _y1, _y2, _y3; \
	_a1 = _hi16(a), _a0 = _lo16(a); \
	_b1 = _hi16(b), _b0 = _lo16(b); \
	_y0 = _a0 * _b0; \
	_y1 = _a0 * _b1; \
	_y2 = _a1 * _b0; \
	_y3 = _a1 * _b1; \
	_y1 += _hi16(_y0);				/* can't carry */ \
	_y1 += _y2;					/* might carry */ \
	if (_y1 < _y2)				\
	_y3 += (CnetUint32)(PR_BIT(16));		/* propagate */ \
	    (r).lo = (_lo16(_y1) << 16) + _lo16(_y0); \
	    (r).hi = _y3 + _hi16(_y1); \
	}

#define int64_UDIVMOD(qp, rp, a, b)	int64_udivmod(qp, rp, a, b)

extern	void int64_udivmod(CnetUint64 *qp, CnetUint64 *rp,
			   CnetUint64 a, CnetUint64 b);

#define int64_DIV(r, a, b) { \
	CnetInt64 _a, _b; \
	CnetUint32 _negative = (CnetInt32)(a).hi < 0; \
	if (_negative) { \
	    int64_NEG(_a, a); \
	} else { \
	    _a = a; \
	} \
	if ((CnetInt32)(b).hi < 0) { \
	    _negative ^= 1; \
	    int64_NEG(_b, b); \
	} else { \
	    _b = b; \
	} \
	int64_UDIVMOD(&(r), 0, _a, _b); \
	if (_negative) \
	    int64_NEG(r, r); \
}

#define int64_MOD(r, a, b) { \
	CnetInt64 _a, _b; \
	CnetUint32 _negative = (CnetInt32)(a).hi < 0; \
	if (_negative) { \
		int64_NEG(_a, a); \
	} else { \
		_a = a; \
	} \
	if ((CnetInt32)(b).hi < 0) { \
		int64_NEG(_b, b); \
	} else { \
		_b = b; \
	} \
	int64_UDIVMOD(0, &(r), _a, _b); \
	if (_negative) \
		int64_NEG(r, r); \
}

#define int64_SHL(r, a, b) { \
	if (b) { \
	CnetInt64 _a; \
	_a = a; \
	if ((b) < 32) { \
		(r).lo = _a.lo << ((b) & 31); \
		(r).hi = (_a.hi << ((b) & 31)) | (_a.lo >> (32 - (b))); \
	} else { \
		(r).lo = 0; \
		(r).hi = _a.lo << ((b) & 31); \
	} \
	} else { \
		(r) = (a); \
	} \
}

/* a is an CnetInt32, b is CnetInt32, r is CnetInt64 */
#define int64_ISHL(r, a, b) { \
	if (b) { \
		CnetInt64 _a; \
		_a.lo = (a); \
		_a.hi = 0; \
		if ((b) < 32) { \
			(r).lo = (a) << ((b) & 31); \
			(r).hi = ((a) >> (32 - (b))); \
		} else { \
			(r).lo = 0; \
			(r).hi = (a) << ((b) & 31); \
		} \
	} else { \
		(r).lo = (a); \
		(r).hi = 0; \
	} \
}

#define int64_SHR(r, a, b) { \
	if (b) { \
		CnetInt64 _a; \
		_a = a; \
		if ((b) < 32) { \
			(r).lo = (_a.hi << (32-(b))) | (_a.lo >> ((b) & 31)); \
			(r).hi = (CnetInt32)_a.hi >> ((b) & 31); \
		} else { \
			(r).lo = (CnetInt32)_a.hi >> ((b) & 31); \
			(r).hi = (CnetInt32)_a.hi >> 31; \
		} \
	} else { \
		(r) = (a); \
	} \
}

#define int64_USHR(r, a, b) { \
	if (b) { \
		CnetInt64 _a; \
		_a = a; \
		if ((b) < 32) { \
			(r).lo = (_a.hi << (32-(b))) | (_a.lo >> ((b) & 31)); \
			(r).hi = _a.hi >> ((b) & 31); \
		} else { \
			(r).lo = _a.hi >> ((b) & 31); \
			(r).hi = 0; \
		} \
	} else { \
		(r) = (a); \
	} \
}

#define int64_L2I(i, l)	((i) = (l).lo)
#define int64_L2UI(ui, l)	((ui) = (l).lo)
#define int64_L2F(f, l)	{ double _d; int64_L2D(_d, l); (f) = (CnetFloat64)_d; }

#define int64_L2D(d, l) { \
	int _negative; \
	CnetInt64 _absval; \
	\
	_negative = (l).hi >> 31; \
	if (_negative) { \
	    int64_NEG(_absval, l); \
	} else { \
	    _absval = l; \
	} \
	(d) = (double)_absval.hi * 4.294967296e9 + _absval.lo; \
	if (_negative) \
	    (d) = -(d); \
}

#define int64_I2L(l, i)	{ CnetInt32 _i = ((CnetInt32)(i)) >> 31; \
				(l).lo = (i); (l).hi = _i; }
#define int64_UI2L(l, ui)	((l).lo = (ui), (l).hi = 0)
#define int64_F2L(l, f)	{ double _d = (double)f; int64_D2L(l, _d); }

#define int64_D2L(l, d) { \
	int _negative; \
	double _absval, _d_hi; \
	CnetInt64 _lo_d; \
	\
	_negative = ((d) < 0); \
	_absval = _negative ? -(d) : (d); \
	\
	(l).hi = _absval / 4.294967296e9; \
	(l).lo = 0; \
	int64_L2D(_d_hi, l); \
	_absval -= _d_hi; \
	_lo_d.hi = 0; \
	if (_absval < 0) { \
		_lo_d.lo = -_absval; \
		int64_SUB(l, l, _lo_d); \
	} else { \
		_lo_d.lo = _absval; \
		int64_ADD(l, l, _lo_d); \
	} \
	\
	if (_negative) \
		int64_NEG(l, l); \
}

#endif /* !HAVE_LONG_LONG */

#endif

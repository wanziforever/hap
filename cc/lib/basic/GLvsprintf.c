/*
**      File ID:        @(#): <MID41907 () - 02/19/00, 9.1.1.1>
**
**	File:			MID41907
**	Release:		9.1.1.1
**	Date:			05/13/00
**	Time:			13:46:16
**	Newest applied delta:	02/19/00
**
** DESCRIPTION:
**      This file contains wrapper functions for the standard 'sprintf()'s.
**	The two entry points are 'GLsprintf()' and 'GLvsprintf()'.
**	Instead of passing a buffer to 'vsprintf()', 
**	they return a 'malloc()'ed buffer to us.
**	The caller is responsible for 'free()'ing the return buffer.
**
** NOTES:
**	The estimation routine is loosely based on code found on the home page 
**	of Hoshi Takanori (hoshi@sra.co.jp), Last update: 1997/02/17 06:25:15
**	In Feb 98, the URL was http://www.sra.co.jp/people/hoshi/cr_sprintf.html
**	(Try a search for 'sprintf', or in this case 'cr_sprintf').
**
**	The original mini-sprintf() code was used as a skeleton
**	for a routine to estimate the length of a 'sprintf()' string.
**	Formatting routines and buffer handlers were replaced
**	by conservative length estimates.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>		/* for va_list, etc. */
#include <ctype.h>		/* for isdigit() */

#ifdef  __cplusplus
extern "C" {
#endif

/* 
 *	When the width or precision is left unspecified, 
 *	this is the default numeric field width.
 *	This value is large enough for a 64-bit integer with sign.
 *	Floating point values allow for twice this length.
 */
#define DEFAULT_WIDTH	20

/*
 *	Integer maximum function
 */
#define	max(A,B)	(((A) > (B)) ? (A) : (B))


/*
 *	Format specification flags
 */
enum flag {
	flag_none  = 0x0000,		/* no flags set */

	/* this next group is presently unused... */
	flag_minus = 0x0001,		/* "-" for justification */
	flag_plus  = 0x0002,		/* "+" for justification */
	flag_space = 0x0004,		/* " " for leading spaces */
	flag_hash  = 0x0008,		/* "#" for alternate leader */
	flag_zero  = 0x0010,		/* "0" for zero-fill */

	flag_short = 0x0020,		/* "%h" for half-long */
	flag_long  = 0x0040,		/* "%l" for long */
	flag_long_long = 0x0080,	/* "%ll" for long long */
	flag_wide  = 0x0100		/* "%wc" or "%ws" for wide chars */
};


/*
 *	Convervatively estimate the length of a formatted string
 */
int
GLvstrlen(const char *format, va_list arg)
{
	char	c;		/* character from format string */
	int	len;		/* accumulated formatted length */
	int	num;		/* width of an individual entry */
	int	width;		/* width specification from format */
	int	prec;		/* precision specification from format */
	int	f;		/* format specification flag(s) */

	/* various argument types */
	long long	long_long_value;
	long	value;
	double	dval;
	char	*sptr;
	void	*vptr;	

	if ( format == NULL ) {
		return -1;
	}

	/* 
	 *	Step through the given format,
	 *	detect formatting specifications,
	 *	accumulate estimated widths
	 */
	len = 0;
	while ( (c = *format++) != '\0' ) {
		if ( c != '%' || (c = *format++) == '%' ) {
			len++;
		} else {

			/* Reset width, precision, and fmt flags */
			width = 0;
			prec = -1;
			f = flag_none;

			/* Skip over leading format flags */
			while ( strchr("-+0 #", c) != NULL ) {
				c = *format++;
			}

			/* Detect variable width, collect it */
			if ( c == '*' ) {
				width = va_arg(arg, int);
				if ( width < 0 ) {
					width = -width;
				}
				c = *format++;
			} else {
				/* collect optional literal width */
				while ( isdigit(c) ) {
					width = width * 10 + (c - '0');
					c = *format++;
				}
			}

			/* Detect start of precision and collect it */
			if ( c == '.' ) {
				c = *format++;
				/* get variable precision */
				if (c == '*') {
					prec = va_arg(arg, int);
					if ( prec < 0 ) {
						prec = 0;
					}
					c = *format++;
				} else {
					/* skip over a negative character */
					if ( c == '-' ) {
						c = *format++;
					}
					/* get optional literal precision */
					prec = 0;
					while ( isdigit(c) ) {
						prec = prec * 10 + (c - '0');
						c = *format++;
					}
				}
			}

			/* Detect "half" and "long" flags */
			if (c == 'h' ) {
				f |= flag_short;
				c = *format++;
			}
			else if (c == 'l') {
				if ( *(format+1) == 'l' ) {
					f |= flag_long_long;
					format++;
				}
				else {
					f |= flag_long;
				}
				c = *format++;
			}

			/*
			 *	Switch on the format character, 
			 *	put the estimated width into 'num'
			 */
			switch (c) {
			case 'd':
			case 'i':
			case 'o':
			case 'u':
			case 'x':
			case 'X':
				/* integers: '%d' '%i' '%o' '%u' '%x' */
				if (f & flag_short) {
					value = (short) va_arg(arg, short);
				} else if (f & flag_long) {
					value = va_arg(arg, long);
				} else if (f & flag_long_long) {
					long_long_value =
						(long long) va_arg(arg, long long);
				} else {
					value = va_arg(arg, int);
				}
				num = max(width, DEFAULT_WIDTH);
				num += 2;	/* leading sign/hex/octal */
				break;

			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				/* floating point: '%e' '%f' '%g' */
				dval = va_arg(arg, double);
				if ( width == 0 ) {
					num = max(prec,  2*DEFAULT_WIDTH);
				} else {
					num = max(width, 2*DEFAULT_WIDTH);
				}
				num += 6;	/* sign, DP, exponent */
				break;

			case 'w':
				/* wide-characters: '%wc' '%ws' */
				f |= flag_wide;
				c = *format++;
				/* and fall through */

			case 'c':
			case 's':
				/* character: '%c' */
				/* string: '%s' */
				if ( c == 'c' ) {
					value = va_arg(arg, int);
					num = 1;
				} else {
					sptr = va_arg(arg, char*);
					if ( sptr == NULL ) {
						num = 8;	/* "(null)" */
					} else {
						num = max(width, strlen(sptr));
					}
				}
				/* 
				 * wide characters take twice the space
				 * (when we support them)
				 */
				if ( f & flag_wide ) {
					num *= 2;
				}
				break;

			case 'p':
				/* address: '%p' "0x12345678" */
				vptr = va_arg(arg, void*);
				num = DEFAULT_WIDTH;
				break;

			case 'n':
				/* output length: '%n' */
				/* 
				 * this doesn't affect the buffer length, 
				 * we just need to bump the arg pointer
				 */
				if (f & flag_short) {
					vptr = (void *) va_arg(arg, short*);
				} else if (f & flag_long) {
					vptr = (void *) va_arg(arg, long*);
				} else {
					vptr = (void *) va_arg(arg, int*);
				}
				break;

			default:
				/* all other characters pass through */
				num = 1;
				break;
			}
			/* accumulate the overall length */
			len += num;
		}
	}
	return len;
}


/*
 *	Wrapper for the standard 'vsprintf()'
 */
char *
GLvsprintf(const char *format, va_list ap)
{
	char *buf = NULL;

	// Linux 64-bit does not like re-use of the ap list, so copy it
	va_list aq;
	va_copy(aq, ap);
	int len = GLvstrlen(format, aq);
	va_end(aq);
	if ( len >= 0 ) {
		buf = (char *)malloc(len+1);
		if ( buf != NULL ) {
			vsprintf(buf, format, ap);
			buf[len] = 0;
		}
	}
	return buf;
}


/*
 *	Wrapper for the standard 'sprintf()'
 */
char *
GLsprintf(const char* format, ...)
{
	char	*buf = NULL;
	va_list ap;
	va_start(ap, format);
	buf = GLvsprintf(format, ap);
	va_end(ap);
	return buf;
}

#ifdef  __cplusplus
}
#endif



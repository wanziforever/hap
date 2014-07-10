
#ifndef	_GLTYPES
#define	_GLTYPES

#define	Char	char			/* signed 8 - bit integer	*/

#define	Short	short			/* signed 16 - bit integer	*/

/* In 64 bit systems, a 32 bit interger is now int, not Long, so even
** though it is a little confusing, Long will now become int.
** Long will still represent a 32 bit integer, inspite of a confusing name.
*/

#define	Long	int			/* signed 32 - bit integer	*/

#define U_char unsigned char		/* unsigned 8 - bit integer	*/

#define U_short unsigned short		/* unsigned 16 - bit integer	*/

#define U_long unsigned int		/* unsigned 32 - bit integer	*/

#ifdef _LP64
#define LongLong  long 			/* signed 64 - bit integer	*/
#define U_LongLong unsigned long 	/* unsigned 64 - bit integer	*/
#else
#define LongLong  long long		/* signed 64 - bit integer	*/
#define U_LongLong unsigned long long	/* unsigned 64 - bit integer	*/
#endif

#define	Void	void	/* mc68 won't typedef anything void.	*/

/* define the boolean type.				*/
#define	NO	0
#define	YES	1

#ifndef TRUE
#define	FALSE	0
#define	TRUE	1
#endif

#define Bool	char

/* Must decide endianness! */
#ifdef __linux

/* This will define __BYTE_ORDER as __BIG_ENDIAN or __LITTLE_ENDIAN */
#include <endian.h>

#else

/* Must be a Sparc machine, which is __BIG_ENDIAN */
#define __BIG_ENDIAN       4321
#define __BYTE_ORDER       __BIG_ENDIAN

#endif

#endif

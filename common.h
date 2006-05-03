/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: common.h
 * Description: Common Definitions
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ---------------------------------------------------------------------------
 */


/* Header protection */
#ifndef COMMON_H
#define COMMON_H

/*
 * Static parameters
 */

/* Package (program) informations */
#define PROGRAM_NAME      "PuZ"
#define PROGRAM_VERSION   "built on " __DATE__ " " __TIME__
#define PROGRAM_STRING    PROGRAM_NAME " " PROGRAM_VERSION
#define PROGRAM_BUGREPORT "b.gaillard@powercode.net"
#define PROGRAM_YEARS     "2006"
#define PROGRAM_AUTHORS   "Lionel Imbs & Benjamin Gaillard"
#define PROGRAM_COPYRIGHT "Copyright (C) " PROGRAM_YEARS " " PROGRAM_AUTHORS
#define PROGRAM_LICENSE   /* GPL */ \
    "This is free software.  " \
    "You may redistribute copies of it under the terms of\n" \
    "the GNU General Public License " \
    "<http://www.gnu.org/licenses/gpl.html>.\n" \
    "There is NO WARRANTY, to the extent permitted by law."

/* The UPC process that is the initiator */
#define INITIATOR ((int) 0)


/*
 * Global macros
 */

/* Disable debugging by default */
#if !defined DEBUG && !defined NDEBUG
# define NDEBUG 1
#endif /* !DEBUG && !NDEBUG */

/* Define __STDC_VERSION__ if it isn't already */
#ifndef __STDC_VERSION__
# define __STDC_VERSION__ 198901L /* C89 */
#endif /* !__STDC_VERSION__ */


/*
 * Language support
 */

/* Add support for the 'restrict' keyword */
#if __STDC_VERSION__ < 199901L /* !C99 */
# ifdef __GNUC__ /* GCC */
#  define restrict __restrict__
# else /* !GCC */
#  define restrict
# endif /* !GCC */
#endif /* !C99 */

/* Add support for the 'bool' datatype */
#ifdef __BERKELEY_UPC__ /* BUPC makes some ugly code transformations */
# define bool  unsigned
# define false 0U
# define true  1U
#elif __STDC_VERSION__ >= 199901L /* C99 */
# include <stdbool.h> /* bool, true, false */
#elif !defined __cplusplus /* C89/90 */
typedef unsigned char bool;
# define false ((bool) (0 == 1))
# define true  ((bool) (0 == 0))
#endif /* C89/90 */

/* Container for a boolean value */
typedef unsigned char bool_t;
#define FALSE ((bool_t) 0)
#define TRUE  ((bool_t) 1)


/*
 * Miscellaneous macros
 */

/* Number of int's (words) to allocate for a table of 'bits' bits */
#define BITS_TO_WORDS(bits) ((((bits) + (sizeof(unsigned) * 8 - 1)) \
			     & ~(sizeof(unsigned) * 8 - 1U)) / 8)

/* Bitfield manipulation */
#define BIT_GET(table, bit) (((table)[(bit) / (sizeof(*(table)) * 8)] \
			     >> ((bit) % (sizeof(*(table)) * 8))) & 1U)
#define BIT_SET(table, bit) ((table)[(bit) / (sizeof(*(table)) * 8)] \
			     |= 1U << ((bit) % (sizeof(*(table)) * 8)))
#define BIT_CLR(table, bit) ((table)[(bit) / (sizeof(*(table)) * 8)] \
			     &= ~(1U << ((bit) % (sizeof(*(table)) * 8))))

/* Header protection */
#endif /* !COMMON_H */

/* End of File */

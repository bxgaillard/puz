/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: ida.h
 * Description: Solver IDA* Algorithm Header
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
#ifndef IDA_H
#define IDA_H

/*
 * Headers
 */

/* Global definitions */
#include "common.h"

/* Other headers */
#include <stdio.h> /* FILE */

/* Local definitions */
#include "util.h" /* dir_t */


/*
 * Datatypes
 */

/* Work giving strategy */
enum giving_strat { GIVING_CYCLE_ASYNC, GIVING_CYCLE_GLOBAL, GIVING_RANDOM };

/* Termination detection strategy */
enum term_strat { TERM_COUNTER, TERM_DIJKSTRA, TERM_TREE };


/*
 * Variables
 */

extern unsigned loop_count, deepening_count, move_count;


/*
 * Function prototypes
 */

extern bool ida_init(const enum giving_strat giv, const enum term_strat term,
		     const unsigned min_length);
extern void ida_free(void);
extern bool ida_loop(unsigned *const restrict length,
		     clock_t *const restrict mean_time);
extern void print_solution(FILE *const restrict output);

/* Header protection */
#endif /* !IDA_H */

/* End of File */

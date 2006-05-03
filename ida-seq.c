/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: ida-seq.c
 * Description: Sequencial Solver IDA* Algorithm Implementation
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


/*
 * Headers
 */

/* Global definitions */
#include "common.h"

/* System headers */
#include <stdlib.h> /* NULL, malloc(), free(), abs() */
#include <stdio.h>  /* FILE, fprintf(), fputs(), putc() */
#include <string.h> /* memcpy(), memcmp(), strerror() */
#include <time.h>   /* clock_t, clock() */
#include <assert.h> /* assert() */

/* Local headers */
#include "input.h" /* tile_t, coord_t */
#include "util.h"  /* enum dir, dir_t, dir_vectors */
#include "ida.h"

/* UPC */
#ifdef __UPC__
# include <upc.h>
# include <upc_relaxed.h>
#endif /* __UPC */


/*
 * Local Datatypes
 */

/* Return value for the test_tree() function */
enum test_res { TEST_FOUND, TEST_NOTFOUND, TEST_MEMORY };


/*
 * Static (Private) Variables
 */

/* Depth limit (current) for tree walkthrough */
static unsigned limit;

/* Current path in IDA* tree and the best one if found */
static dir_t *restrict path;

/* Current matrix; work is done on this one */
static tile_t *restrict current;

/* Start and minimum computed Manhattan summation for an IDA* iteration */
static unsigned start_man, min_man;


/*
 * Global Variables
 */

/* Statistics */
unsigned loop_count, deepening_count, move_count;


/*
 * Static (Private) Functions
 */

/**
 * Move through a tree within specified limits.
 *
 * @param limit the tree deepening depth limit.
 *
 * @return TEST_FOUND if a solution was found, TEST_NOTFOUNT if not or
 *         TEST_MEMORY if there wasn't enough memory.
 */
static enum test_res test_tree(const unsigned limit)
{
    /* Local variables */
    unsigned depth;                        /* Current depth in tree */
    struct state *const restrict state     /* State stack */
	    = (struct state *) malloc((limit) * sizeof(struct state));
    static const enum dir dir_opp[DIR_NUM] /* Opposite directions */
	    = { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT };

    /* Verify memory allocation */
    if (state == NULL)
	return TEST_MEMORY;
    if ((path = (dir_t *) malloc(limit * sizeof(dir_t))) == NULL) {
	free(state);
	return TEST_MEMORY;
    }

    /* Initialize tables */
    state[0].pos = (tile_t) blank_pos, state[0].line = (coord_t) blank_line,
    state[0].col = (coord_t) blank_col, state[0].man = start_man,
    path[0] = DIR_FIRST;

    /* Test everything */
    for (depth = 0;;) {
	deepening_count++; /* Another deepening loop */

	if (path[depth] > DIR_LAST) {
	    /* Every direction tested for this step, move backward */
	    if (depth > 0) {
		undo(current, state, depth--);
		path[depth]++;
	    } else
		break;
	/* Check if we aren't moving backward then do the move */
	} else if ((depth == 0 || path[depth] != dir_opp[path[depth - 1]])
		   && move(current, state, (enum dir) path[depth],
			   depth, limit, &min_man, false)) {
	    move_count++; /* This is another move */

	    /* Move OK, check for completeness */
	    if (++depth == limit) {
		free(state);
		return TEST_FOUND;
	    }

	    /* Initialize next move */
	    path[depth] = (dir_t) DIR_FIRST;
	} else
	    /* Try another move */
	    path[depth]++;
    }

    /* Free memory, we haven't found anything yet */
    free(state);
    free(path);
    return TEST_NOTFOUND;
}


/*
 * Global (Public) Functions
 */

/**
 * Allocate memory and initialize data for the IDA* loop.
 *
 * @return TRUE on success; false otherwise (not enough memory).
 */
bool ida_init(const enum giving_strat giv, const enum term_strat term,
	      const unsigned min_length)
{
    /* Allocate memory */
    if ((current = (tile_t *) malloc(size * sizeof(tile_t))) == NULL)
	return false;

    /* Initialize current matrix to the start one */
    memcpy(current, start, size * sizeof(tile_t));

    /* Compute start matrix Manhattan heuristic */
    start_man = manhattan(start);

    /* Inhibits warnings about unused parameters */
    (void) giv, (void) term, (void) min_length;

    /* No error */
    return true;
}

/**
 * Free the memory used by the IDA* loop.
 *
 * @return void.
 */
void ida_free(void)
{
    free(current);
    if (path != NULL)
	free(path);
}

/**
 * The main IDA* (Iterative Deepening A*) loop.
 *
 * @param length the found path length (returned value).
 *
 * @return wether a solution has been found.
 */
bool ida_loop(unsigned *const restrict length,
	      clock_t *const restrict mean_time)
{
    const clock_t clock_start = clock();

    /* If the start and final matrices are the same */
    if (start_man == 0) {
	*length = 0, limit = 0;
	return true;
    }

    /* Assertions */
    assert(length != NULL);

    /* Initialize variables */
    limit = start_man, min_man = 0,
    loop_count = 0, deepening_count = 0, move_count = 0;

    /* The loop */
    for (;;) {
	/* Another IDA* loop */
	printf("IDA* loop iteration #%u: using a limit of %u.\n",
	       ++loop_count, limit);

	/* Walk through the tree */
	switch (test_tree(limit)) {
	case TEST_FOUND:
#ifndef NDEBUG
	    replay(current, path, limit);
	    assert(memcmp(current, final, size * sizeof(tile_t)) == 0);
#endif /* !NDEBUG */
	    *length = limit;
	    *mean_time = clock() - clock_start;
	    return true;

	case TEST_MEMORY:
	    *length = (unsigned) -1;
	    return false;

	case TEST_NOTFOUND:
	    break;
	}

	/* Adjust limit using a Manhattan summation heuristic */
	limit += min_man;
	min_man = 0;
    }

    /* No solution found within limits */
    /* Unreachable code -- return false; */
}

/**
 * Print the best found path on the specified output.
 *
 * @param output the output stream.
 *
 * @return void.
 */
void print_solution(FILE *const restrict output)
{
    print_path(path, limit, output);
}

/* End of File */

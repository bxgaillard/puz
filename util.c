/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: util.h
 * Description: Utility Functions Implementation
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

/* Global definitinos */
#include "common.h"

/* System headers */
#include <stdlib.h> /* NULL */
#include <stdio.h>  /* putc(), fprintf(), fputs() */
#include <string.h> /* memcpy() */
#include <assert.h> /* assert() */

/* Local headers */
#include "input.h"
#include "util.h"


/*
 * Global variables
 */

/* Vectors for each direction */
struct dir_vector dir_vectors[DIR_NUM] = {
    /* 'pos' for 'up' and 'down' cannot be determined at compile time so
       they're updated in the util_init() function */
    {            42,             1,             0 }, /* Up    */
    {            42, (unsigned) -1,             0 }, /* Down  */
    {             1,             0,             1 }, /* Left  */
    { (unsigned) -1,             0, (unsigned) -1 }  /* Right */
};

/* Position (coordinates) of each tile in the final matrix */
struct pos *restrict final_pos;


/*
 * Global (public) functions
 */

/**
 * Initialize the utility module.
 *
 * @return true on success, false if there wasn't enough memory.
 */
bool util_init(void)
{
    /* Local variables */
    unsigned i, j, k; /* Counters */
    unsigned tile;    /* Current tile id */

    if ((final_pos = (struct pos *) malloc(size * sizeof(struct pos)))
	== NULL)
	return false;

    /* Create tile position table for Manhattan summation computation */
    for (i = 0, k = 0; i < height; i++)
	for (j = 0; j < width; j++, k++) {
	    tile = final[k];
	    final_pos[tile].i = (coord_t) i, final_pos[tile].j = (coord_t) j;
	}

    /* Initialize vectors */
    dir_vectors[DIR_UP].pos = width,
    dir_vectors[DIR_DOWN].pos = (unsigned) -(int) width;

    return true;
}

/**
 * Free the memory used by the utility module.
 *
 * @return void.
 */
void util_free(void)
{
    free(final_pos);
}

/**
 * Compute the Manhattan distances summation between the current and the final
 * matrices.
 *
 * @param matrix the concerned matrix.
 *
 * @return the result of the computation.
 */
unsigned manhattan(const tile_t *const restrict matrix)
{
    /* Local variables */
    unsigned i, j, k;         /* Counters */
    unsigned tile, total = 0; /* Tile id, distance sum */

    /* Assertions */
    assert(matrix != NULL && final_pos != NULL);
    assert(width >= 2 && height >= 2);

    /* Compute the result */
    for (i = 0, k = 0; i < height; i++)
	for (j = 0; j < width; j++, k++) {
	    if ((tile = matrix[k]) == BLANK_TILE)
		continue;

	    total += (unsigned) abs((signed) i - (signed) final_pos[tile].i)
		   + (unsigned) abs((signed) j - (signed) final_pos[tile].j);
	}

    /* Return the result */
    return total;
}

/**
 * Try to do a move.
 *
 * @param matrix   the matrix on which we move.
 * @param state    the state stack.
 * @param dir      the direction to try.
 * @param depth    the current deepening depth.
 * @param limit    the deepening depth limit.
 * @param simulate if true, don't actually do the move: only test it.
 *
 * @return true if the move was successful; false otherwise.
 */
bool move(tile_t *const restrict matrix, struct state *restrict state,
	  enum dir dir, unsigned depth, const unsigned limit,
	  unsigned *const restrict min_man, const bool simulate)
{
    /* Local variables */
    unsigned pos, line, col;             /* Current (old) coordinates */
    unsigned new_pos, new_line, new_col; /* New coordinates */
    unsigned new_man;                    /* New Manhattan heuristic */
    tile_t tile;                         /* Moved tile */
    int final_line, final_col;           /* Final tile coordinates */

    /* Assertions */
    assert(matrix != NULL && state != NULL);
    assert(depth < limit);
    assert(state[depth].pos < size);
    assert(state[depth].line < height && state[depth].col < width);
    assert(matrix[state[depth].pos] == BLANK_TILE);
    assert(/* always true -- dir >= DIR_FIRST && */ dir <= DIR_LAST);
    assert(state[depth].man <= limit);
    assert(min_man != NULL);

    /* Test if we can move: blank tile must be in the matrix */
    if ((new_pos = (pos = state[depth].pos) + dir_vectors[dir].pos) < size
	&& (new_col = (col = state[depth].col) + dir_vectors[dir].col)
	   < width) {
	/* Calculate the line coordinate in addition to position and column */
	new_line = (line = state[depth].line) + dir_vectors[dir].line;

	/* Get information for new Manhattan heuristic calculation */
	tile = matrix[new_pos];
	final_line = (signed) final_pos[tile].i,
	final_col  = (signed) final_pos[tile].j;

	/* Compute the new Manhattan heuristic */
	new_man = (unsigned) ((int) state[depth].man
			      + abs((signed) line - final_line)
			      + abs((signed) col - final_col)
			      - abs((signed) new_line - final_line)
			      - abs((signed) new_col - final_col));

	/* Test the depth limit */
	if (new_man == 0)
	    return true;
	if (depth + new_man < limit) {
	    if (!simulate) {
		depth++;      /* We moved further! */

		/* Update matrix */
		matrix[pos] = matrix[new_pos];
		matrix[new_pos] = BLANK_TILE;

		/* Store new data in the state */
		state[depth].pos  = (tile_t) new_pos,
		state[depth].line = (coord_t) new_line,
		state[depth].col  = (coord_t) new_col,
		state[depth].man  = new_man;
	    }

	    return true; /* No problem during the move */
	} else if (*min_man == 0 || new_man < *min_man)
	    /* Limit reached, this heuristic is a candidate for next loop */
	    *min_man = new_man;
    }

    return false; /* Move not possible */
}

/**
 * Undo a move.
 *
 * @param matrix the matrix on which we move.
 * @param state the state stack.
 * @param depth the current deepening depth.
 *
 * @return void.
 */
void undo(tile_t *const restrict matrix, struct state *restrict state,
	  unsigned depth)
{
    /* Assertions */
    assert(matrix != NULL && state != NULL);
    assert(depth > 0);
    assert(state[depth].pos < size && state[depth - 1].pos < size);
    assert(matrix[state[depth].pos] == BLANK_TILE);

    /* Undo the move in the work matrix */
    matrix[state[depth].pos] = matrix[state[depth - 1].pos];
    matrix[state[depth - 1].pos] = BLANK_TILE;
}

/**
 * Replay moves stored in a path table from the start matrix.
 *
 * @param matrix the matrix on which we move.
 * @param path  the steps (path) to replay.
 * @param depth the number of steps (path length).
 *
 * @return void.
 */
void replay(tile_t *const restrict matrix, const dir_t *const restrict path,
	    const unsigned length)
{
    /* Local variables */
    unsigned i;                   /* Index in path */
    int pos = blank_pos, new_pos; /* Current and next positions */

    /* Assertions */
    assert(path != NULL);
    assert(pos >= 0 && pos < (int) size);

    /* Restore work matrix to the beginning state */
    memcpy(matrix, start, size * sizeof(tile_t));

    /* Walk the path, updating work matrix */
    for (i = 0; i < length; i++) {
	/* Get direction and new position */
	new_pos = pos + dir_vectors[path[i]].pos;

	/* Consistency check */
	assert(matrix[pos] == 0);
	assert(new_pos >= 0 && new_pos < (int) size);

	/* Update current matrix after a consistency check */
	assert(matrix[pos] == 0);
	matrix[pos] = matrix[new_pos];
	matrix[new_pos] = 0;
	pos = new_pos;
    }
}

/**
 * Print a path on the specified output.
 *
 * @param path   the path to display.
 * @param length the path length.
 * @param output the output stream.
 *
 * @return void.
 */
void print_path(const dir_t *const restrict path, const unsigned length,
		FILE *const restrict output)
{
    /* Local variables */
    unsigned i;                      /* Counter */
    static const char dirs[DIR_NUM]  /* Direction characters */
	    = { 'U', 'D', 'L', 'R' };

    /* Assertions */
    assert(path != NULL);
    assert(output != NULL);

    /* Print path */
    for (i = 0; i < length; i++)
	putc(dirs[path[i] % DIR_NUM], output);
    putc('\n', output);
}

#if 0 /* Not used anymore */
/**
 * Print a matrix on the specified output.
 *
 * @param matrix the matrix to display.
 * @param output the output stream.
 *
 * @return void.
 */
void print_matrix(tile_t *const restrict matrix,
		  FILE *const restrict output)
{
    unsigned i, j, k; /* Counters */

    /* Assertions */
    assert(matrix != NULL && output != NULL);
    assert(width >= 2 && height >= 2);

    /* Header */
    for (i = 0; i < width; i++)
	fputs("+-----", output);
    fputs("+\n", output);

    /* Dump matrix */
    for (i = 0, k = 0; i < height; i++) {
	for (j = 0; j < width; j++, k++)
	    if (matrix[k] != BLANK_TILE)
		fprintf(output, "| %3d ", matrix[k]);
	    else
		fputs("| [_] ", output);
	fputs("|\n", output);

	for (j = 0; j < width; j++)
	    fputs("+-----", output);
	fputs("+\n", output);
    }
}
#endif

/* End of File */

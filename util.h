/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: util.h
 * Description: Utility Functions Header
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
#ifndef UTIL_H
#define UTIL_H

/*
 * Headers
 */

/* Global definitions */
#include "common.h"

/* Other headers */
#include <stdio.h> /* FILE */
#include "input.h" /* tile_t */


/*
 * Datatypes
 */

/* Direction */
enum dir {
    DIR_FIRST = 0,
    DIR_UP = 0, DIR_DOWN, DIR_LEFT, DIR_RIGHT,
    DIR_LAST = 3, DIR_NUM, DIR_UNKNOWN = 4
};

/* Container for a direction */
typedef unsigned char dir_t;

struct dir_vector { unsigned pos, line, col; };

/* Element for the state stack */
struct state {
    tile_t   pos;       /* Current position: matrix index */
    coord_t  line, col; /* Current position: coordinates */
    unsigned man;       /* Manhattan heuristic for this step */
};

/* Position in the matrix (i = 0..height-1, j = 0..width-1 */
struct pos { coord_t i, j; };


/*
 * Global variables
 */

extern struct dir_vector dir_vectors[DIR_NUM];

/* Position (coordinates) of each tile in the final matrix */
extern struct pos *restrict final_pos;


/*
 * Function prototypes
 */

extern bool util_init(void);
extern void util_free(void);
extern unsigned manhattan(const tile_t *const restrict matrix);
extern bool move(tile_t *const restrict matrix, struct state *restrict state,
		 enum dir dir, unsigned depth, const unsigned limit,
		 unsigned *const restrict min_man, const bool simulate);
extern void undo(tile_t *const restrict matrix, struct state *restrict state,
		 unsigned depth);
extern void replay(tile_t *const restrict matrix,
		   const dir_t *const restrict path, const unsigned length);
extern void print_path(const dir_t *const restrict path,
		       const unsigned length, FILE *const restrict output);
/* Not used anymore
extern void print_matrix(tile_t *const restrict matrix,
			 FILE *const restrict output); */

/* Header protection */
#endif /* !UTIL_H */

/* End of File */

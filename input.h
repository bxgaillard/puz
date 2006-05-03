/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: input.h
 * Description: Input Reading Functions
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
#ifndef INPUT_H
#define INPUT_H

/*
 * Headers
 */

/* Global definitions */
#include "common.h"


/*
 * Datatypes
 */

/* Tile id and coordinate element */
typedef unsigned short tile_t;
typedef unsigned char  coord_t;


/*
 * Macros
 */

/* Constant representing a blank tile */
#define BLANK_TILE ((tile_t) 0)


/*
 * Variables
 */

extern unsigned width, height, size;
extern unsigned blank_pos, blank_line, blank_col;
extern tile_t *restrict start, *restrict final;


/*
 * Functions prototypes
 */

extern bool read_data(FILE *const restrict input, const bool square);
extern void free_data(void);

/* Header protection */
#endif /* !INPUT_H */

/* End of File */

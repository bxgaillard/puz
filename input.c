/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: input.c
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


/*
 * Headers
 */

/* Global definitions */
#include "common.h"

/* System headers */
#include <stdlib.h> /* NULL, malloc(), free() */
#include <stdio.h>  /* FILE, stderr, fprintf(), fscanf() */
#include <string.h> /* memset() */
#include <assert.h> /* assert() */

/* Local headers */
#include "input.h"


/*
 * Global Variables
 */

/* Matrices dimensions, number of tiles (including the blank one) */
unsigned width, height, size;

/* Start position of the blank tile */
unsigned blank_pos, blank_line, blank_col;

/* Start and final matrices */
tile_t *restrict start, *restrict final;


/*
 * Static (Private) Functions
 */

/**
 * Read a tile matrix from the given input stream.
 *
 * @param input     the input stream.
 * @param set_blank true if blank_pos should be set; false otherwise.
 *
 * @return the created matrix or NULL if an error occured.
 */
static tile_t *read_matrix(FILE *const restrict input, const bool set_blank)
{
    /* Local variables */
    const size_t set_size = BITS_TO_WORDS(size); /* Tile set size */
    tile_t *const restrict matrix                /* The matrix */
	    = (tile_t *) malloc(size * sizeof(tile_t));
    unsigned data;                               /* Read integer */
    unsigned *restrict set;                      /* Tile set */
    unsigned i, line, col;                       /* Counter and coordinates */

    /* Assertions */
    assert(input != NULL);
    assert(width >= 2 && height >= 2);
    assert(size % width == 0);

    /* Verify memory allocation */
    if (matrix == NULL) {
	fputs("Error: out of memory.\n", stderr);
	return NULL;
    }

    /* Initialize cursor and tile set */
    set = (unsigned *) malloc(set_size);
    memset(set, 0, set_size);

    /* Read tiles */
    for (line = 0, i = 0; line < height; line++)
	for (col = 0; col < width; col++, i++) {
	    /* Read the integer */
	    if (fscanf(input, "%u", &data) != 1) {
		fputs("Error: illegal data in input.\n", stderr);
		break;
	    }

	    /* Do some checks */
	    if (data >= size) {
		/* Out of bounds */
		fprintf(stderr, "Error: illegal tile id %u.\n", data);
		break;
	    }
	    if (BIT_GET(set, data)) {
		/* Duplicate id */
		fprintf(stderr, "Error: tile id %u used more than once.\n",
			data);
		break;
	    }
	    BIT_SET(set, data); /* Add id to list */

	    /* Update blank tile position */
	    if (data == BLANK_TILE && set_blank)
		blank_pos = i, blank_line = line, blank_col = col;

	    /* Store data */
	    matrix[i] = (tile_t) data;
	}

    /* Free memory */
    free(set);

    /* In case of error */
    if (i != size) {
	free(matrix);
	return NULL;
    }

    /* Return the resulting matrix */
    return matrix;
}

/**
 * Read data (two matrices and their dimensions) from the given input stream.
 *
 * @param input  the input stream.
 * @param square if matrices are square ones (width == height).
 *
 * @return true if no error occured, false otherwise.
 */
bool read_data(FILE *const restrict input, const bool square)
{
    /* Assertions */
    assert(input != NULL);

    /* Read matrices size */
    if (square) {
	if (fscanf(input, "%u", &width) != 1) {
	    fputs("Illegal data in input.\n", stderr);
	    return false;
	}
	height = width;
    } else if (fscanf(input, "%u%u", &width, &height) != 2) {
	fputs("Illegal data in input.\n", stderr);
	return false;
    }

    /* Check given matrices width and height */
    if (width < 2 || height < 2) {
	fprintf(stderr, "Error: illegal matrix size (%u x %u)\n",
		width, height);
	return false;
    }
    if (width > 1U << (sizeof(coord_t) * 8) ||
	height > 1U << (sizeof(coord_t) * 8)) {
	fprintf(stderr, "Error: matrix size (%u x %u) exceeds coord_t "
			"capacity (%u).\n",
		width, height, 1U << (sizeof(coord_t) * 8));
	return false;
    }
    if (width * height > 1U << (sizeof(tile_t) * 8)) {
	fprintf(stderr,
		"Error: tile number (%u) exceeds tile_t capacity (%u).\n",
		width * height, 1U << (sizeof(tile_t) * 8));
	return false;
    }

    /* Compute matrices size (number of tiles, including the blank one) */
    size = width * height;

    /* Read start and final matrices from input */
    if ((start = read_matrix(input, true)) == NULL)
	return false;
    if ((final = read_matrix(input, false)) == NULL) {
	free(start);
	return false;
    }

    /* No error */
    return true;
}

/**
 * Free previously allocated memory for input data storage.
 *
 * @return void.
 */
void free_data(void)
{
    free(start);
    free(final);
}

/* End of File */

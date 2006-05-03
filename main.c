/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: main.c
 * Description: Main Function
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
#include <stdlib.h> /* NULL, atoi */
#include <stdio.h>  /* FILE, std{in,out,err}, f*(), printf(), putc() */
#include <string.h> /* memcmp(), strcmp(), strerror() */
#include <errno.h>  /* errno */
#include <time.h>   /* clock_t, CLOCKS_PER_SEC */
#include <assert.h> /* assert() */

/* UPC headers */
#ifdef __UPC__
# include <upc.h>
# include <upc_relaxed.h>
#endif /* __UPC__ */

/* Local headers */
#include "input.h"
#include "util.h"
#include "ida.h"


/*
 * Macos
 */

/* Exit function */
#ifdef __UPC__
# undef exit
# define exit upc_global_exit
#endif /* __UPC__ */


/*
 * Static (Private) Functions
 */

/**
 * Get the argument for an option.
 *
 * @param argc main()'s argument count.
 * @param argv main()'s argument vector.
 * @param arg  argv index of the option.
 * @param chr  character of the option in argv[arg].
 *
 * @return the option or NULL of not found.
 */
static const char *get_arg(const int argc,
			   const char *const restrict *const restrict argv,
			   int *const restrict arg, int *const restrict chr)
{
    /* Local variables */
    const char *res; /* Result */

    if (argv[*arg][*chr + 1] != '\0') {
	/* Argument right after option */
	res = &argv[*arg][*chr + 1];
	*chr = -1;
    } else if (*arg < argc - 1) {
	/* Argument is the next one after the option */
	res = argv[*arg + 1];
	(*arg)++, *chr = -1;
    } else {
	/* Missing argument */
	fprintf(stderr, "Error: missing argument to option '-%c'.\n",
		argv[*arg][*chr]);
	res = NULL;
    }

    return res; /* Return option */
}


/*
 * Global (Public) Functions
 */

/**
 * Main function.
 *
 * @return the program exit code: 0 on success, 1 on argument error, 2 on data
 *         read error, 3 on memory allocation error.
 */
int main(const int argc, const char *const restrict *const restrict argv)
{
    int arg, chr;
    const char *restrict input_fn = NULL;
    const char *restrict output_fn = NULL;
    FILE *restrict input, *restrict output;
    clock_t duration;
    bool square = true, quiet = false, found;
    unsigned length;
    int min_length = 20;
    enum giving_strat giving = GIVING_CYCLE_ASYNC;
    enum term_strat term = TERM_COUNTER;
#ifdef __UPC__
    const char *restrict str;
#endif /* __UPC__ */

    /* Assertions */
    assert(argc > 0);
    assert(argv != NULL && argv[0] != NULL);

    /* Parse options */
    for (arg = 1; arg < argc; arg++)
	if (argv[arg][0] == '-')
	    for (chr = 1; chr != 0 && argv[arg][chr] != '\0'; chr++)
		switch (argv[arg][chr]) {
#ifdef __UPC__      /* UPC algorithms parameters options */
		case 'm': /* Minimum work path length */
		    if ((str = get_arg(argc, argv, &arg, &chr)) == NULL)
			upc_global_exit(1);
		    if ((min_length = atoi(str)) < 1) {
			if (MYTHREAD == INITIATOR)
			    fputs("Error: '-m' option accepts only an integer"
				  " greater than or equal to 1.\n", stderr);
			upc_global_exit(1);
		    }
		    break;

		case 'w': /* Work acquisition */
		    if ((str = get_arg(argc, argv, &arg, &chr)) == NULL)
			upc_global_exit(1);
		    if (strcmp(str, "async") == 0)
			giving = GIVING_CYCLE_ASYNC;
		    else if (strcmp(str, "global") == 0)
			giving = GIVING_CYCLE_GLOBAL;
		    else if (strcmp(str, "random") == 0)
			giving = GIVING_RANDOM;
		    else {
			if (MYTHREAD == INITIATOR)
			    fputs("Error: '-w' option accepts 'async', "
				  "'global' or 'random' only.\n", stderr);
			upc_global_exit(1);
		    }
		    break;

		case 't': /* Termination detection */
		    if ((str = get_arg(argc, argv, &arg, &chr)) == NULL)
			upc_global_exit(1);
		    if (strcmp(str, "counter") == 0)
			term = TERM_COUNTER;
		    else if (strcmp(str, "dijkstra") == 0)
			term = TERM_DIJKSTRA;
		    else if (strcmp(str, "tree") == 0)
			term = TERM_TREE;
		    else {
			if (MYTHREAD == INITIATOR)
			    fputs("Error: '-t' option accepts 'counter', "
				  "'dijkstra' or 'tree' only.\n", stderr);
			upc_global_exit(1);
		    }
		    break;
#endif /* __UPC__ */

		    /* Input/output files */
		case 'i': /* Input */
		    if ((input_fn = get_arg(argc, argv, &arg, &chr)) == NULL)
			exit(1);
		    break;
		case 'o': /* Output */
		    if ((output_fn = get_arg(argc, argv, &arg, &chr)) == NULL)
			exit(1);
		    break;
		case 'r': /* Rectangle matrices */
		    square = false;
		    break;

		    /* Miscellaneous options */
		case 'q': /* Quiet */
		    quiet = true;
		    break;
		case 'h': /* Help */
#ifdef __UPC__
		    if (MYTHREAD == INITIATOR)
#endif /* __UPC__ */
			printf("Syntax: %s [options...]\n\n"
			       "Options:\n"
#ifdef __UPC__                 /* UPC-specific options */
			       "    -m length: minimum path length of given "
				   "work\n"
			       "    -w method: work acquisition method: "
				   "'async'*, 'global' or 'random'\n"
			       "    -t method: termination detection "
				   "method: 'counter'*, 'dijkstra' or "
				   "'tree'\n"
#endif /* __UPC__ */
			       "    -i input:  input file (standard input"
				   " by default)\n"
			       "    -o output: output file (standard "
				   "output by default)\n"
			       "    -r:        rectangle (not square) "
				   "matrices in input\n"
			       "    -q:        quiet mode (just print "
				   "result, no statistics)\n"
			       "    -h:        print this help message\n"
			       "    -v:        print " PROGRAM_NAME
				   " version\n\n"
#ifdef __UPC__                 /* UPC-specific options */
			       "Note: '*' describes a default value.\n\n"
#endif /* __UPC__ */
			       "Please report bugs to <" PROGRAM_BUGREPORT
			       ">.\n", argv[0]);
		    exit(0);
		case 'v': /* Version */
#ifdef __UPC__
		    if (MYTHREAD == INITIATOR)
#endif /* __UPC__ */
			puts(PROGRAM_STRING "\n" PROGRAM_COPYRIGHT "\n\n"
			     PROGRAM_LICENSE);
		    exit(0);
		default: /* Unknown option */
#ifdef __UPC__
		    if (MYTHREAD == INITIATOR)
#endif /* __UPC__ */
			fprintf(stderr,
				"Error: unknown option '-%c'.\n"
				"Use '-h' for syntax help.\n",
				argv[arg][chr]);
		    exit(1);
		}
	else {
	    /* Argument which isn't an option */
#ifdef __UPC__
	    if (MYTHREAD == INITIATOR)
#endif /* __UPC__ */
		fprintf(stderr, "Error: incorrent argument '%s'.\n"
				"Use '-h' for syntax help.\n", argv[arg]);
	    exit(1);
	}

    /* Open files */
    if (input_fn != NULL) {
	if (input_fn[0] == '-' && input_fn[1] == '\0')
	    input_fn = NULL, input = stdin; /* "-" = stdin */
	else if ((input = fopen(input_fn, "r")) == NULL) {
	    fprintf(stderr, "Error: cannot read '%s': %s.\n",
		    input_fn, strerror(errno));
	    exit(1);
	}
    } else
	input = stdin;

    if (
#ifdef __UPC__
	MYTHREAD == INITIATOR &&
#endif /* __UPC__ */
	output_fn != NULL) {
	if (output_fn[0] == '-' && output_fn[1] == '\0')
	    output_fn = NULL, output = stdout; /* "-" = stdout */
	else if ((output = fopen(output_fn, "w")) == NULL) {
	    fprintf(stderr, "Error: cannot write to '%s': %s.\n",
		    output_fn, strerror(errno));
	    exit(1);
	}
    } else
	output = stdout;

    /* Input data */
    if (!read_data(input, square))
	exit(2);
    if (!util_init()) {
	fputs("Error: out of memory.\n", stderr);
	free_data();
	exit(3);
    }
    if (input_fn != NULL)
	fclose(input);

    /* Initialize the IDA* loop */
    if (!ida_init(giving, term, (unsigned) min_length)) {
	fputs("Error: out of memory.\n", stderr);
	free_data();
	util_free();
	exit(3);
    }

    /* Run the IDA* (Iterative Deepening A*) loop */
    if ((found = ida_loop(&length, &duration)) == false) {
	fputs("Error: out of memory.\n", stderr);
	free_data();
	ida_free();
	exit(3);
    }

    /* Print results */
#ifdef __UPC__
    if (MYTHREAD == INITIATOR) {
#endif /* __UPC__ */
	if (!quiet) {
	    printf("\nNumber of main IDA* loop iterations: %u\n"
#ifndef __UPC__
		   "Number of tree deepening steps: %u\nNumber of moves: %u\n"
#endif /* !__UPC__ */
		   "Mean duration: %ld.%03ld s\n",
		   loop_count,
#ifndef __UPC__
		   deepening_count, move_count,
#endif /* !__UPC__ */
		   duration / CLOCKS_PER_SEC,
		   duration / (CLOCKS_PER_SEC / 1000) % 1000);
	    if (found) {
		printf("Move number (path length): %u\nMoves: ", length);
		print_solution(stdout);
	    } else
		/* Should never happen */
		puts("No solution found within limits.");
	}

	/* Put result in the output file if specified */
	if (output_fn != NULL || quiet) {
	    if (found)
		print_solution(output);
	    else {
		putc('-', output);
		putc('\n', output);
	    }
	}
#ifdef __UPC__
    }
#endif /* __UPC__ */

    /* Free memory and exit program */
    if (output_fn != NULL)
	fclose(output);
    free_data();
    util_free();
    ida_free();

#ifdef __UPC__
    upc_barrier;
    upc_global_exit(0);
#endif /* __UPC__ */
    return 0;
}

/* End of File */

/* ---------------------------------------------------------------------------
 *
 * PuZ: (N^2 - 1)-Puzzle Solver
 * Copyright (C) 2006 Lionel Imbs & Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: ida-prl.upc
 * Description: Parallel Solver IDA* Algorithm Implementation
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
#include <stdlib.h> /* NULL, malloc(), free(), rand(), srand(), abort() */
#include <stdio.h>  /* FILE, fprintf(), fputs(), putc() */
#include <string.h> /* memcpy(), memcmp(), strerror() */
#include <time.h>   /* clock_t, time(), clock() */
#include <assert.h> /* assert() */

/* Local headers */
#include "input.h" /* tile_t, coord_t */
#include "util.h"  /* enum dir, dir_t, dir_vectors */
#include "ida.h"

/* UPC */
#include <upc.h>
#include <upc_relaxed.h>


/*
 * Macros
 */

/* Integer '1.0' for the tree termination detection */
#define TERM_TREE_WEIGHT_1 (1U << (sizeof(unsigned) * 8 - 1))


/*
 * Local Datatypes
 */

/* Return value for the test_tree() function */
enum test_res { TEST_FOUND, TEST_NOTFOUND, TEST_MEMORY };

/* Local termination detection */
enum term { TERM_WORKING, TERM_END, TERM_FOUND };
typedef unsigned char term_t;

/* Color for the Dijkstra termination detection method */
enum color { COLOR_WHITE, COLOR_BLACK };
enum token { TOKEN_OUT_THERE, TOKEN_WHITE, TOKEN_BLACK, TOKEN_END };
typedef unsigned char token_t;


/*****************************************************************************
 *
 * Termination Detection Functions
 *
 */

/*
 * Local Variables
 */

/* Strategy */
static enum term_strat term_strat;

/* Termination condition */
static shared int term_counter;
static upc_lock_t *term_lock;

/* Weight, for the tree method */
static shared unsigned weight[THREADS];
static volatile unsigned *restrict my_weight;
static int last_giver;

/* Color and token for the Dijkstra method */
static enum color my_color;
static shared token_t token[THREADS];

/* Wether a solution has been found */
static shared term_t all_term[THREADS];
static volatile term_t *restrict my_term;
static upc_lock_t *found_lock;

/* Anticipated declaration of the below variable */
static upc_lock_t *work_lock[THREADS];


/*
 * Static (Private) Functions
 */

/**
 * Broadcast a value indicating termination.
 *
 * @param term the termination reason: either TERM_END or TERM_FOUND.
 *
 * @return void.
 */
static void term_broadcast(const enum term reason)
{
    /* Local variables */
    int thread; /* Thread id */

    /* Assertions */
    assert(reason == TERM_END || reason == TERM_FOUND);
    assert(*my_term == TERM_WORKING);

    /* Send termination flag to all threads */
    for (thread = 0; thread < THREADS; thread++)
	all_term[thread] = (term_t) reason;
    upc_fence;
}

/**
 * Function called in the beginning of each IDA* loop iteration.
 *
 * @return void.
 */
static void term_init_iteration(void)
{
    *my_term = (term_t) TERM_WORKING; /* I am now working */

    switch (term_strat) {
    case TERM_COUNTER:
	/* Initialize counter if I'm the initiator */
	if (MYTHREAD == INITIATOR)
	    term_counter = 1;
	break;

    case TERM_DIJKSTRA:
	/* Initialize my color and my token (I have it if I'm initiator) */
	my_color = COLOR_WHITE;
	token[MYTHREAD] = MYTHREAD == INITIATOR
			? TOKEN_BLACK : TOKEN_OUT_THERE;
	break;

    case TERM_TREE:
	/* Initiator's weight is null in the beginning */
	if (MYTHREAD == INITIATOR)
	    *my_weight = 0U;
	break;

    default:
	abort(); /* Should never happen */
    }
}

/**
 * Function called when some work has been given to another thread.
 *
 * @param thread the id of the thread which received the work.
 *
 * @return void.
 */
static void term_gave_work(const int thread)
{
    /* Assertions */
    assert(thread >= 0 && thread != MYTHREAD);

    switch (term_strat) {
    case TERM_COUNTER:
	/* Increment global counter */
	upc_lock(term_lock);
	term_counter++;
	upc_unlock(term_lock);
	break;

    case TERM_DIJKSTRA:
	/* Update color if receiver < myself (giver) */
	if ((thread - INITIATOR) % THREADS
	    < (MYTHREAD - INITIATOR) % THREADS)
	    my_color = COLOR_BLACK;
	break;

    case TERM_TREE:
	/* Give weight to receiver */
	weight[thread] = (MYTHREAD == INITIATOR && *my_weight == 0)
		       ? (*my_weight = TERM_TREE_WEIGHT_1 / 2)
		       : (*my_weight /= 2);
	break;

    default:
	abort(); /* Should never happed */
    }
}

/**
 * Function called when a work has been finished.
 *
 * @return void.
 */
static void term_finished_work(void)
{
    upc_fence; /* Get pending data if any */

    if (*my_term == TERM_WORKING) /* Only if not already terminated */
	switch (term_strat) {
	case TERM_COUNTER:
	    /* Decrement counter */
	    upc_lock(term_lock);
	    if (--term_counter == 0U)
		term_broadcast(TERM_END);
	    else
		upc_fence;
	    upc_unlock(term_lock);
	    break;

	case TERM_DIJKSTRA:
	    /* Everything is handled in term_check() below */
	    break;

	case TERM_TREE:
	    if (MYTHREAD == INITIATOR) {
		if (*my_weight == 0U) {
		    /* No work has been given by the initiator */
		    *my_weight = TERM_TREE_WEIGHT_1;
		    term_broadcast(TERM_END);
		}
	    } else {
		/* Give back weight */
		upc_lock(work_lock[last_giver]);
		upc_lock(work_lock[MYTHREAD]);
		if ((weight[last_giver] += *my_weight) == TERM_TREE_WEIGHT_1)
		    term_broadcast(TERM_END);
		else
		    upc_fence;
		*my_weight = 0U;
		upc_unlock(work_lock[MYTHREAD]);
		upc_unlock(work_lock[last_giver]);
	    }
	    break;

	default:
	    abort(); /* Should never happen */
	}
}

/**
 * Check wether the termination condition is verified.
 *
 * @return true if all threads are idle; false otherwise.
 */
static bool term_check(void)
{
    /* Local variables */
    token_t my_token; /* My own token */

    /* If a result has been found, work is over */
    upc_fence; /* Get pending data if any */
    if (*my_term != (term_t) TERM_WORKING)
	return true;

    switch (term_strat) {
    case TERM_COUNTER:
	/* Nothing to do here */
	break;

    case TERM_DIJKSTRA:
	/* If the initiator receives a white token, it's finished */
	if (MYTHREAD == INITIATOR
	    && token[MYTHREAD] == (token_t) TOKEN_WHITE) {
	    token[MYTHREAD] = TOKEN_END;
	    term_broadcast(TERM_END);
	    return true;
	}

	/* Transmit token and update its color if appropriate */
	if ((my_token = token[MYTHREAD]) != TOKEN_OUT_THERE) {
	    if (MYTHREAD == INITIATOR)
		my_token = (token_t) TOKEN_WHITE;
	    else if (my_color == COLOR_BLACK)
		my_token = (token_t) TOKEN_BLACK, my_color = COLOR_WHITE;
	    token[MYTHREAD] = (token_t) TOKEN_OUT_THERE;
	    token[(MYTHREAD + 1) % THREADS] = my_token;
	    upc_fence;
	}
	break;

    case TERM_TREE:
	/* Give back weight if we have any */
	upc_lock(work_lock[MYTHREAD]);
	if (*my_weight != 0U) {
	    upc_lock(work_lock[last_giver]);
	    if ((weight[last_giver] += *my_weight) == TERM_TREE_WEIGHT_1)
		term_broadcast(TERM_END);
	    else
		upc_fence;
	    *my_weight = 0U;
	    upc_unlock(work_lock[last_giver]);
	}
	upc_unlock(work_lock[MYTHREAD]);
	break;

    default:
	abort(); /* Should never happen */
    }

    return false;
}


/*****************************************************************************
 *
 * Work Acquisition Functions
 *
 */

/*
 * Local Variables
 */

/* Strategy */
static enum giving_strat giving_strat;

/* Workers which wait for some work */
static shared int all_worker[THREADS];
static volatile int *restrict my_worker;
/* Declared upwards -- static upc_lock_t *work_lock[THREADS]; */

/* The path and its length for giving work */
static shared unsigned all_path_length[THREADS];
static shared [] dir_t *shared all_paths[THREADS];

/* The work giver (target) */
static int my_target;
static shared int global_target;
static upc_lock_t *target_lock;


/*
 * Constants
 */

/* Special return values of the get_work() function */
#define WORK_NONE ((unsigned) -1) /* There isn't work on the elected thread */
#define WORK_END  ((unsigned) -2) /* End of an IDA* loop iteration */


/*
 * Static (Private) Functions
 */

/**
 * Get some work from another thread.
 *
 * @param limit the limit (heuristic) of the current IDA* loop.
 *
 * @return the path prefix length of the gotten work; -1 if there isn't
 *         anymore from the remote thread, -2 if the current IDA* loop is
 *         already over.
 */
static unsigned get_work(const unsigned limit)
{
    /* Local variables */
    int giver;
    unsigned length;
    static unsigned last_limit = (unsigned) -1;

    /* If we are the first thread and this is a new IDA* iteration */
    if (MYTHREAD == INITIATOR && limit != last_limit) {
	last_limit = limit;
	return 0; /* Return a blank path */
    }

    /* Check if not already terminated; no mutex needed here */
    if (term_check())
	return WORK_END;

    /* Elect a thread for getting work */
    assert(giving_strat == GIVING_CYCLE_ASYNC
	   || giving_strat == GIVING_CYCLE_GLOBAL
	   || giving_strat == GIVING_RANDOM);
    do
	switch (giving_strat) {
	case GIVING_CYCLE_ASYNC:  /* Asynchronous cycle */
	    my_target = ((giver = my_target) + 1) % THREADS;
	    break;

	case GIVING_CYCLE_GLOBAL: /* Global cycle */
	    upc_lock(target_lock);
	    global_target = ((giver = global_target) + 1) % THREADS;
	    upc_unlock(target_lock);
	    break;

	case GIVING_RANDOM:       /* Random */
	    giver = rand() % THREADS;
	    break;

	default:                  /* Should never happen... */
	    abort();
	}
    while (giver == MYTHREAD); /* Cannot elect ourselves */

    /* Announce ourselves to the giving thread and wait its acknowledge */
    all_path_length[MYTHREAD] = (unsigned) -1;
    upc_fence; /* Ensure data is written */

    /* Get access to the giving thread */
    upc_lock(work_lock[giver]);
    if (all_worker[giver] == -1) {
	upc_unlock(work_lock[giver]);
	return WORK_NONE;
    }
    all_worker[giver] = MYTHREAD;

    /* Wait for the data to come */
    while ((length = all_path_length[MYTHREAD]) == (unsigned) -1)
	upc_fence; /* Active wait */

    if (length > 0) { /* We have work! (How lucky ;-) */
	/* Let the others get work from the giving thread */
	upc_unlock(work_lock[giver]);

	/* Inform the user too */
	printf("Thread %d acquired work from thread %d.\n", MYTHREAD, giver);
	/*print_path((dir_t *) all_paths[MYTHREAD], length, stdout);*/

	/* Register the last giver and return result */
	if (term_strat == TERM_TREE)
	    last_giver = giver;
	return length;
    }

    /* Let access to others and return */
    upc_unlock(work_lock[giver]);
    return WORK_NONE;
}

/**
 * Give some work to another thread if requested.
 *
 * @param path  the current examined path.
 * @param next  the table of the next directions to try.
 * @param limit the maximum path (prefix) length to offer.
 *
 * @return the length of the path (prefix) given to the remote thread.
 */
static unsigned give_work(dir_t *const restrict path,
			  dir_t *const restrict next,
			  const bool *const restrict explore,
			  unsigned depth, const unsigned limit)
{
    /* Local variables */
    int candidate; /* The requesting thread */

    /* Assertions */
    assert(path != NULL && next != NULL);

    upc_fence; /* Get pending data if any */

    if (*my_worker != -1) {
	if (depth <= limit) {
	    /* Try to give work to each demanding thread */
	    /* Actually, one thread per iteration only */
	    if /*while*/ ((candidate = *my_worker) != MYTHREAD) {
		/* Reset thread id */
		*my_worker = MYTHREAD;

		/* Find a prefix */
		for (; depth <= limit; depth++) {
		    for (; next[depth] <= DIR_LAST; next[depth]++)
			if (explore[depth * DIR_NUM + next[depth]])
			    break;
		    if (next[depth] <= DIR_LAST)
			break;
		}

		/* If found, send it */
		if (depth <= limit) {
		    if (depth > 0)
			upc_memput(all_paths[candidate], path,
				    depth * sizeof(dir_t));
		    all_paths[candidate][depth] = next[depth]++;

		    /* Inform termination detector that we gave some work */
		    term_gave_work(candidate);

		    upc_fence; /* Send path length in the end only */
		    all_path_length[candidate] = depth + 1;
		} else
		    /* No more prefix */
		    all_path_length[candidate] = 0;
		upc_fence; /* Be sure to send/receive data */
	    }
	} else {
	    /* Reset thread id */
	    candidate = *my_worker;
	    *my_worker = -1;

	    /* If a thread is waiting for work, inform him we haven't any */
	    if (candidate != MYTHREAD) {
		all_path_length[candidate] = 0;
		upc_fence;
	    }
	}
    }

    return depth; /* Return the minimum path prefix length */
}


/*****************************************************************************
 *
 * Main IDA* Functions for the Local Thread
 *
 */

/*
 * Global Variables
 */

/* Statistics */
unsigned loop_count, deepening_count, move_count;


/*
 * Local Variables
 */

/* Current matrix; work is done on this one */
static tile_t *restrict current;

/* Minimum path length for given work */
static unsigned min_length;

/* Minimum Manhattan heuristic */
static unsigned min_man;

/* Minimum computed Manhattan summation for an IDA* iteration */
static shared unsigned all_min_man[THREADS];

/* Best path: solution, stored on thread 0 */
static shared unsigned best_length;
static shared [] dir_t *best_path;

/* Execution duration */
static shared clock_t duration[THREADS];


/*
 * Static (Private) Functions
 */

/**
 * Move through a tree within specified limits.
 *
 * @param path  the current path we work on.
 * @param depth the depth at which the search begins (the path prefix length).
 * @param limit the tree deepening depth limit.
 *
 * @return TEST_FOUND if a solution was found, TEST_NOTFOUNT if not or
 *         TEST_MEMORY if there wasn't enough memory.
 */
static enum test_res test_tree(dir_t *const restrict path,
			       unsigned depth, const unsigned limit,
			       const unsigned start_man)
{
    /* Local variables */
    unsigned next_depth;                   /* Current depth in tree */
    struct state *const restrict state     /* State stack */
	    = (struct state *) malloc(limit * sizeof(struct state));
    dir_t *const restrict next             /* Next direction (for threads) */
	    = path + limit;
    bool *const restrict explore           /* Directions to explore */
	    = (bool *) malloc(limit * DIR_NUM * sizeof(bool));
    enum dir dir;                          /* Direction */
    bool success;                          /* Wether moving was successful */
    static const enum dir dir_opp[DIR_NUM] /* Opposite directions */
	    = { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT };

    /* Verify memory allocation */
    if (state == NULL || explore == NULL) {
	if (state == NULL)
	    free(state);
	if (explore == NULL)
	    free(explore);
	return TEST_MEMORY;
    }

    /* Initialize tables */
    state[0].pos = (tile_t) blank_pos, state[0].line = (coord_t) blank_line,
    state[0].col = (coord_t) blank_col, state[0].man = start_man;

    /* Do the replay */
    memcpy(current, start, size * sizeof(tile_t));
    for (next_depth = 0; next_depth < depth; next_depth++) {
	assert(next_depth == 0
	       || path[next_depth] != dir_opp[path[next_depth - 1]]);
	success = move(current, state, (enum dir) path[next_depth],
		       next_depth, limit, &min_man, false);
	assert(success);
	/*move_count++;*/
	next[next_depth] = (dir_t) DIR_LAST + 1;
    }

    /* Initialize paths to explore */
    for (dir = DIR_FIRST; dir <= DIR_LAST; dir++)
	explore[depth * DIR_NUM + dir]
		= (depth == 0 || dir != dir_opp[path[depth - 1]])
		&& move(current, state, dir, depth, limit, &min_man, true);

    /* Initialize path */
    path[depth] = (dir_t) DIR_FIRST, next[depth] = (dir_t) DIR_FIRST + 1;

    /* We have work to give */
    *my_worker = MYTHREAD;

    /* Test everything */
    assert(next_depth == depth);
    while (*my_term != (term_t) TERM_FOUND) {
	/*deepening_count++;*/

	if ((next_depth == 0 ? start_man : state[next_depth - 1].man)
	    >= min_length)
	    next_depth = give_work(path, next, explore, next_depth, depth);
	else
	    give_work(path, next, explore, 1, 0);

	if (path[depth] > DIR_LAST) {
	    /* Every direction tested for this step, move backward */
	    if (depth > next_depth) {
		undo(current, state, depth--);
		path[depth] = next[depth]++;
	    } else
		break;
	/* Check if we aren't moving backward then do the move */
	} else if (explore[depth * DIR_NUM + path[depth]]
		   && move(current, state, (enum dir) path[depth],
			   depth, limit, &min_man, false)) {
	    /*move_count++;*/ /* This is another move */

	    /* Move OK, check for completeness */
	    if (++depth == limit) {
		if (upc_lock_attempt(found_lock)) {
		    term_broadcast(TERM_FOUND);
		    best_length = limit;
		    upc_memput(best_path, path, limit * sizeof(dir_t));
		}
		break;
	    }

	    /* Initialize next move */
	    for (dir = DIR_FIRST; dir <= DIR_LAST; dir++)
		explore[depth * DIR_NUM + dir]
			= dir != dir_opp[path[depth - 1]]
			&& move(current, state, dir, depth, limit, &min_man,
				true);
	    path[depth] = (dir_t) DIR_FIRST;
	    next[depth] = (dir_t) DIR_FIRST + 1;
	} else
	    /* Try another move */
	    path[depth] = next[depth]++;
    }

    /* "Tell" the others we don't have work to give anymore */
    give_work(path, next, explore, 1, 0);

    /* We finished a work unit */
    printf("Thread %d finished its work.\n", MYTHREAD);
    term_finished_work();

    /* Free memory */
    free(state);
    free(explore);
    return depth == limit ? TEST_FOUND : TEST_NOTFOUND;
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
	      const unsigned min_len)
{
    /* Local variables */
    int thread; /* Thread id */

    /* Assertions */
    assert(giv == GIVING_CYCLE_ASYNC || giv == GIVING_CYCLE_GLOBAL
	   || giv == GIVING_RANDOM);
    assert(term == TERM_COUNTER
	   || term == TERM_DIJKSTRA || term == TERM_TREE);

    /* Allocate memory */
    if ((current = (tile_t *) malloc(size * sizeof(tile_t))) == NULL)
	return false;

    /* Initialize shared variables and locks for completion detection */
    if ((found_lock = upc_all_lock_alloc()) == NULL) {
	free(current);
	return false;
    }
    my_term = (term_t *) &all_term[MYTHREAD];

    /* Initialize shared variables and locks for work acquisition */
    my_worker = (int *) &all_worker[MYTHREAD];
    *my_worker = -1;
    for (thread = 0; thread < THREADS; thread++)
	if ((work_lock[thread] = upc_all_lock_alloc()) == NULL) {
	    free(current);
	    return false;
	}

    /* Initialize work acquisition strategy */
    switch (giving_strat = giv) {
    case GIVING_CYCLE_ASYNC:
	my_target = (MYTHREAD + 1) % THREADS;
	break;

    case GIVING_CYCLE_GLOBAL:
	if ((target_lock = upc_all_lock_alloc()) == NULL) {
	    free(current);
	    return false;
	}
	global_target = INITIATOR;
	break;

    case GIVING_RANDOM:
	srand(time(NULL) + MYTHREAD);
    }

    /* Initialize termination detection variables and locks */
    switch (term_strat = term) {
    case TERM_DIJKSTRA:
	break;

    case TERM_TREE:
	my_weight = (unsigned *) &weight[MYTHREAD];
	break;

    case TERM_COUNTER:
	if ((term_lock = upc_all_lock_alloc()) == NULL) {
	    free(current);
	    return false;
	}
	break;

    default:
	abort();
    }

    /* Initialize minimum path length for given work */
    min_length = min_len;

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
    /* Local variables */
    int thread; /* Thread id */

    /* Free memory */
    free(current);
    if (MYTHREAD == INITIATOR)
	upc_free(best_path);

    /* Free locks */
    if (MYTHREAD == INITIATOR) {
	upc_lock_free(found_lock);
	for (thread = 0; thread < THREADS; thread++)
	    upc_lock_free(work_lock[thread]);
	if (giving_strat == GIVING_CYCLE_GLOBAL)
	    upc_lock_free(target_lock);
	if (term_strat == TERM_COUNTER)
	    upc_lock_free(term_lock);
    }
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
    /* Local variables */
    int thread;           /* Thread id */
    dir_t *restrict path; /* Current path */
    unsigned limit;       /* Current path length limit */
    unsigned start_man;   /* Manhattan heuristic value for start position */
    unsigned new_man;     /* Current Manhattan heuristic value */
    unsigned depth_start; /* Length of the acquired work (prefix) */
    clock_t clock_start;  /* Start timestamp */
    clock_t clock_total;  /* Total duration time */

    /* Assertions */
    assert(length != NULL);

    /* If the start and final matrices are the same */
    if ((start_man = manhattan(start)) == 0) {
	path = (dir_t *) NULL;
	return true;
    }

    /* Initialize variables */
    limit = start_man, min_man = 0,
    loop_count = 0, deepening_count = 0, move_count = 0;

    /* Save start timestamp */
    clock_start = clock();

    /* The loop */
    for (;;) {
	/* Allocate shared memory */
	path = (dir_t *)
	       (all_paths[MYTHREAD] = (shared [] dir_t *)
				      upc_alloc(limit * sizeof(dir_t) * 2));
	best_path = (shared [] dir_t *)
		    upc_all_alloc(1, limit * sizeof(dir_t));
	term_init_iteration();

	/* Ensure everyone has access to allocated and initialized memory */
	upc_barrier 1;

	/* Another iteration */
	if (MYTHREAD == INITIATOR)
	    printf("IDA* loop iteration #%u: using a limit of %u.\n",
		   ++loop_count, limit);

	/* While there is work to do (no termination condition) */
	while ((depth_start = get_work(limit)) != WORK_END) {
	    /* Get work */
	    while (depth_start == WORK_NONE)
		depth_start = get_work(limit);

	    /* If we got work */
	    if (depth_start != WORK_END) {
		/* Walk through the tree */
		switch (test_tree(path, depth_start, limit, start_man)) {
		case TEST_FOUND:
#ifndef NDEBUG
		    replay(current, path, limit);
		    assert(memcmp(current, final, size * sizeof(tile_t))
			   == 0);
#endif /* !NDEBUG */
		    break;

		case TEST_MEMORY:
		    upc_free(all_paths[MYTHREAD]);
		    *length = (unsigned) -1;
		    return false;

		case TEST_NOTFOUND:
		    break;
		}
	    } else
		break;
	}

	/* Free previously allocated shared memory for this thread's path */
	upc_free(all_paths[MYTHREAD]);

	/* If a solution has been found, the program is over */
	if (*my_term == (term_t) TERM_FOUND)
	    break;

	/* Compute the new minimum Manhattan heuristic */
	all_min_man[MYTHREAD] = min_man;
	upc_notify 2;

	/* Free previously allocated shared memory for the best path */
	if (MYTHREAD == INITIATOR)
	    upc_free(best_path);

	/* Wait for everyone to update their min_man variable */
	upc_wait 2;

	if (MYTHREAD == INITIATOR) {
	    /* Get the minimum value */
	    for (thread = 0; thread < THREADS; thread++)
		if (thread != INITIATOR) {
		    new_man = all_min_man[thread];
		    if (new_man != 0 && (min_man == 0 || new_man < min_man))
			min_man = new_man;
		}

	    /* Copy value to each thread */
	    for (thread = 0; thread < THREADS; thread++)
		all_min_man[thread] = min_man;
	}
	upc_barrier 3; /* Ensure initiator sent the variable to all others */
	if (MYTHREAD != INITIATOR)
	    min_man = all_min_man[MYTHREAD];
	assert(min_man != 0);

	/* Adjust limit using a Manhattan summation heuristic */
	limit += min_man;
	min_man = 0;
    }

    duration[MYTHREAD] = clock() - clock_start;
    upc_barrier 4;
    if (MYTHREAD == INITIATOR) {
	/* Get the mean value */
	clock_total = 0;
	for (thread = 0; thread < THREADS; thread++)
	    clock_total += duration[thread];
	clock_total /= THREADS;

	/* Copy value to each thread */
	for (thread = 0; thread < THREADS; thread++)
	    duration[thread] = clock_total;
    }
    upc_barrier 5;

    /* Solution found */
    *length = limit;
    *mean_time = duration[MYTHREAD];
    return true;
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
    /* Assertions */
    assert(MYTHREAD == INITIATOR);
    assert(output != NULL);

    /* Finally print the path */
    print_path((dir_t *) best_path, best_length, output);
}

/* End of File */

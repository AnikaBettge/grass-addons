#ifndef LOCAL_PROTO_H
#define LOCAL_PROTO_H

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/stats.h>
#include "../r.pi.library/r_pi.h"

#ifdef MAIN
#define GLOBAL
#else
#define GLOBAL extern
#endif

#define TYPE_NOTHING 0
#define TYPE_NOGO -1

#define RESOLUTION 10000

#define MIN_DOUBLE -1000000
#define MAX_DOUBLE 1000000

typedef struct
{
    int x, y;
    int neighbors;
    double value;
} Coords;

typedef struct
{
    int x, y;
} Point;

typedef DCELL(*f_statmethod) (DCELL *, int);
typedef int (*f_func) (DCELL *, Coords **, int, f_statmethod);

/* frag.c */
void writeFragments(int *flagbuf, int nrows, int ncols, int nbr_cnt);

/* func.c */
void f_distance(DCELL * vals, Coords ** frags, int count,
		f_statmethod statmethod);
void f_area(DCELL * vals, Coords ** frags, int count,
	    f_statmethod statmethod);

/* statmethods.c */
DCELL sum(DCELL * vals, int count);
DCELL average(DCELL * vals, int count);

/* global parameters */
GLOBAL int verbose;
GLOBAL Coords **fragments;
GLOBAL Coords *cells;
GLOBAL int fragcount;

#endif /* LOCAL_PROTO_H */

#ifndef LOCAL_PROTO_H
#define LOCAL_PROTO_H

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/stats.h>
#include <math.h>
#include <time.h>

#define NOTHING 0

#define RESOLUTION 10000

#define MIN_DOUBLE -1000000
#define MAX_DOUBLE 1000000

typedef struct
{
    int x, y;
    int neighbors;
    DCELL value;
} Coords;

typedef struct
{
    int x, y;
} Point;

typedef struct
{
    int x, y;
    int patch;
} PatchPoint;

typedef DCELL(*f_statmethod) (DCELL *, int);

/* helpers.c */
int Round(double d);

int Random(int max);

double Randomf();

void print_buffer(int *buffer, int sx, int sy);

void print_d_buffer(DCELL * buffer, int sx, int sy);

void print_map(double *map, int size);

void print_array(DCELL * buffer, int size);

void print_fragments();

/* frag.c */
void writeFragments(DCELL * flagbuf, int nrows, int ncols, int nbr_cnt);

/* stat_method.c */
DCELL average(DCELL * vals, int count);

DCELL variance(DCELL * vals, int count);

DCELL std_deviat(DCELL * vals, int count);

DCELL median(DCELL * vals, int count);

DCELL min(DCELL * vals, int count);

DCELL max(DCELL * vals, int count);

/* global variables */
int verbose;

Coords **fragments;

Coords *cells;

int fragcount;

int sx, sy;

int *id_map;

int *adj_matrix;

#endif /* LOCAL_PROTO_H */

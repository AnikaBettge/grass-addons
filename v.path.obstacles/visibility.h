/****************************************************************
 * MODULE:     v.path.obstacles
 *
 * AUTHOR(S):  Maximilian Maldacker
 *  
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/
#ifndef VISIBILITY_H
#define VISIBILITY_H


#include <stdlib.h>
#include <assert.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/Vect.h>
#include "rotation_tree.h"

int load_lines( struct Map_info *map, struct Point ** points, struct Line ** lines );
int construct_visibility( struct Point * points, struct Line * lines, int num_lines, struct Map_info* out );
int cmp_points(const void* v1, const void* v2);

int turn_left( struct Point p1, struct Point p2, struct Point p3 );

void handle( struct Point* p, struct Point* q, struct Map_info * out );
void report( struct Point * p, struct Point * q, struct Map_info * out );

void init_vis( struct Point * points, struct Line * lines, int num );

struct Point * pop();
struct Point * top();
void push(struct Point * p);
int empty_stack();
void init_stack();

static int stack_index;
static struct Point ** stack;
static struct Point * p_ninfinity;
static struct Point * p_infinity;

#endif

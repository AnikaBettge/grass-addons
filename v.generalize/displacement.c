
/****************************************************************
 *
 * MODULE:     v.generalize
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Methods for displacement
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "point.h"
#include "matrix.h"

/* snakes method modified for displacement.
 * Function returns somthing */
int snakes_displacement(struct Map_info *In, struct Map_info *Out,
			double threshold, double alfa, double beta, double gama,
			double delta, int iterations)
{

    int n_points;
    int n_lines;

    /*this is not THE final version
     * 
     * Also, we must be able to specify the lines we want to
     * displace.
     * 
     * Moreover, there is no reason to cnsider the lines/points
     * which are OK
     * 
     * I just want to see the results, before i spend
     * two afternoons working...:)
     */

    int i, j, index, pindex, iter;
    int with_z = 0;
    struct line_pnts *Points, *Write;
    struct line_cats *Cats;
    MATRIX k, dx, dy, fx, fy, kinv, dx_old, dy_old;
    POINT *parray;
    POINT *pset;
    int *point_index;
    int *first, *line_index;
    double threshold2;
    int from, to;

    Points = Vect_new_line_struct();
    Write = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    n_lines = Vect_get_num_lines(In);
    n_points = 0;

    for (i = 1; i <= n_lines; i++) {
	Vect_read_line(In, Points, NULL, i);
	n_points += Points->n_points;
    };

    parray = (POINT *) G_calloc(n_points, sizeof(POINT));
    pset = (POINT *) G_calloc(n_points, sizeof(POINT));
    point_index = (int *)G_calloc(n_points, sizeof(int));
    first = (int *)G_calloc(n_points, sizeof(int));
    line_index = (int *)G_calloc(n_points, sizeof(int));

    /* read points 
     * TODO: some better/faster method for determining whether two points are the same */
    index = 0;
    pindex = 0;
    from = to = 0;
    for (i = 1; i <= n_lines; i++) {
	Vect_read_line(In, Points, NULL, i);
	from = to;
	to = from + Points->n_points;
	for (j = 0; j < Points->n_points; j++) {
	    int q, w;
	    double a, b, c, h;
	    POINT cur;
	    int findex;
	    point_assign(Points, j, with_z, &cur);
	    /* check whether we alerady have point with the same
	     * coordinates */
	    findex = pindex;
	    for (q = 0; q < pindex; q++)
		if (point_dist_square(cur, pset[q]) < 0.5) {
		    findex = q;
		    break;
		};

	    point_index[index] = findex;
	    if (findex == pindex) {
		point_assign(Points, j, with_z, &pset[pindex]);
		pindex++;
	    };
	    first[index] = (j == 0);
	    line_index[index] = i;
	    point_assign(Points, j, with_z, &parray[index]);
	    index++;
	};
    };

    matrix_init(pindex, pindex, &k);
    matrix_init(pindex, 1, &dx);
    matrix_init(pindex, 1, &dy);
    matrix_init(pindex, 1, &fx);
    matrix_init(pindex, 1, &fy);
    matrix_init(pindex, 1, &dx_old);
    matrix_init(pindex, 1, &dy_old);

    matrix_mult_scalar(0.0, &k);

    double a = 2.0 * alfa + 6.0 * beta;
    double b = -alfa - 4.0 * beta;
    double c = beta;

    /* build matrix */
    for (i = 0; i < index; i++) {
	int r = point_index[i];
	int l = line_index[i];
	k.a[r][r] += a;
	if (i + 1 < index && line_index[i + 1] == l)
	    k.a[r][point_index[i + 1]] += b;
	if (i + 2 < index && line_index[i + 2] == l)
	    k.a[r][point_index[i + 2]] += c;
	if (i >= 1 && line_index[i - 1] == l)
	    k.a[r][point_index[i - 1]] += b;
	if (i >= 2 && line_index[i - 2] == l)
	    k.a[r][point_index[i - 2]] += c;
    };

    //    matrix_print(k);

    threshold2 = threshold * threshold;

    matrix_add_identity(gama, &k);

    matrix_mult_scalar(0.0, &dx);
    matrix_mult_scalar(0.0, &dy);

    /*calculate the inverse */
    G_message(_("Inverting matrix, be patient ..."));
    if (!matrix_inverse(k, &kinv, 4))
	G_fatal_error(_("Could not calculate the inverse matrix"));

    for (iter = 0; iter < iterations; iter++) {

	int conflicts = 0;

	matrix_mult_scalar(0.0, &fx);
	matrix_mult_scalar(0.0, &fy);

	matrix_mult_scalar(0.0, &dx_old);
	matrix_mult_scalar(0.0, &dy_old);

	matrix_add(dx_old, dx, &dx_old);
	matrix_add(dy_old, dy, &dy_old);

	/* calculate force vectors */
	for (i = 0; i < index; i++) {

	    double cx, cy;
	    cx = dx.a[point_index[i]][0];
	    cy = dy.a[point_index[i]][0];
	    double f = sqrt(cx * cx + cy * cy);
	    f /= threshold2;
	    fx.a[point_index[i]][0] -= cx * f;
	    fy.a[point_index[i]][0] -= cy * f;

	    for (j = 1; j < index; j++) {
		if (line_index[i] == line_index[j] || first[j] ||
		    point_index[i] == point_index[j])
		    continue;
		/* if ith point is close to some segment then
		 * apply force to ith point. If the distance
		 * is zero, do not move the points */
		double d, pdist;
		POINT in;
		int status;
		d = dig_distance2_point_to_line(parray[i].x, parray[i].y,
						parray[i].z, parray[j].x,
						parray[j].y, parray[j].z,
						parray[j - 1].x,
						parray[j - 1].y,
						parray[j - 1].z, with_z, &in.x,
						&in.y, &in.z, &pdist, &status);

		POINT dir;
		if (d == 0.0 || d > threshold2)
		    continue;
		d = sqrt(d);
		point_subtract(parray[i], in, &dir);
		point_scalar(dir, 1.0 / d, &dir);
		point_scalar(dir, 1.0 - d / threshold, &dir);
		fx.a[point_index[i]][0] += dir.x;
		fy.a[point_index[i]][0] += dir.y;
		conflicts++;
	    };
	};
	printf("Conflicts: %d\n", conflicts);

	matrix_mult_scalar(delta, &fx);
	matrix_mult_scalar(delta, &fy);
	matrix_mult_scalar(gama, &dx);
	matrix_mult_scalar(gama, &dy);

	matrix_add(dx, fx, &fx);
	matrix_add(dy, fy, &fy);

	matrix_mult(kinv, fx, &dx);
	matrix_mult(kinv, fy, &dy);

	for (i = 0; i < index; i++) {
	    parray[i].x +=
		dx.a[point_index[i]][0] - dx_old.a[point_index[i]][0];
	    parray[i].y +=
		dy.a[point_index[i]][0] - dy_old.a[point_index[i]][0];
	};


    };
    index = 0;
    for (i = 1; i <= n_lines; i++) {
	int type = Vect_read_line(In, Points, Cats, i);
	/*Vect_read_line(In, Write, NULL, i);
	 * Write->n_points = 2; */
	for (j = 0; j < Points->n_points; j++) {
	    /*  Write->x[0] = Points->x[j];
	     * Write->y[0] = Points->y[j]; */
	    Points->x[j] = parray[index].x;
	    Points->y[j] = parray[index].y;
	    /*            Write->x[1] = Points->x[j];
	     * Write->y[1] = Points->y[j];
	     * Vect_write_line(Out, GV_LINE, Write, Cats); */
	    index++;
	};
	Vect_write_line(Out, GV_LINE, Points, Cats);
    };

    G_free(parray);
    G_free(pset);
    G_free(point_index);
    G_free(first);
    G_free(line_index);
    matrix_free(k);
    matrix_free(kinv);
    matrix_free(dx);
    matrix_free(dy);
    matrix_free(fx);
    matrix_free(fy);
    matrix_free(dx_old);
    matrix_free(dy_old);
    return 0;
};

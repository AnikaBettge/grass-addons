
/****************************************************************
 *
 * MODULE:     v.net.allpairs
 *
 * AUTHOR(S):  Daniel Bundala
 *
 * PURPOSE:    Shortest paths between all nodes
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
#include <grass/dbmi.h>
#include <grass/neta.h>

int main(int argc, char *argv[])
{
    struct Map_info In, Out;
    static struct line_pnts *Points;
    struct line_cats *Cats;
    char *mapset;
    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *map_in, *map_out;
    struct Option *cat_opt, *field_opt, *where_opt, *abcol, *afcol;
    struct Flag *geo_f, *newpoints_f;
    int chcat, with_z;
    int layer, mask_type;
    VARRAY *varray;
    dglGraph_s *graph;
    int i, j, geo, nnodes, nlines, max_cat, *cats;
    dglInt32_t **dist;
    char buf[2000];

    /* Attribute table */
    dbString sql;
    dbDriver *driver;
    struct field_info *Fi;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    module->keywords = _("network, shortest path, all pairs");
    module->description =
	_("Computes the shortest path between all pairs of nodes.");

    /* Define the different options as defined in gis.h */
    map_in = G_define_standard_option(G_OPT_V_INPUT);
    map_out = G_define_standard_option(G_OPT_V_OUTPUT);

    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    cat_opt = G_define_standard_option(G_OPT_V_CATS);
    where_opt = G_define_standard_option(G_OPT_WHERE);

    afcol = G_define_option();
    afcol->key = "afcolumn";
    afcol->type = TYPE_STRING;
    afcol->required = NO;
    afcol->description = _("Arc forward/both direction(s) cost column");

    abcol = G_define_option();
    abcol->key = "abcolumn";
    abcol->type = TYPE_STRING;
    abcol->required = NO;
    abcol->description = _("Arc backward direction cost column");

    geo_f = G_define_flag();
    geo_f->key = 'g';
    geo_f->description =
	_("Use geodesic calculation for longitude-latitude locations");

    newpoints_f = G_define_flag();
    newpoints_f->key = 'a';
    newpoints_f->description = _("Add points on nodes without points");

    /* options and flags parser */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    /* TODO: make an option for this */
    mask_type = GV_LINE | GV_BOUNDARY;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    Vect_check_input_output_name(map_in->answer, map_out->answer,
				 GV_FATAL_EXIT);

    if ((mapset = G_find_vector2(map_in->answer, "")) == NULL)
	G_fatal_error(_("Vector map <%s> not found"), map_in->answer);

    Vect_set_open_level(2);

    if (1 > Vect_open_old(&In, map_in->answer, mapset))
	G_fatal_error(_("Unable to open vector map <%s>"),
		      G_fully_qualified_name(map_in->answer, mapset));

    with_z = Vect_is_3d(&In);

    if (0 > Vect_open_new(&Out, map_out->answer, with_z)) {
	Vect_close(&In);
	G_fatal_error(_("Unable to create vector map <%s>"), map_out->answer);
    }

    if (geo_f->answer) {
	geo = 1;
	if (G_projection() != PROJECTION_LL)
	    G_warning(_("The current projection is not longitude-latitude"));
    }
    else
	geo = 0;

    /* parse filter option and select appropriate lines */
    layer = atoi(field_opt->answer);
    if (where_opt->answer) {
	if (layer < 1)
	    G_fatal_error(_("'%s' must be > 0 for '%s'"), "layer", "where");
	if (cat_opt->answer)
	    G_warning(_
		      ("'where' and 'cats' parameters were supplied, cat will be ignored"));
	chcat = 1;
	varray = Vect_new_varray(Vect_get_num_lines(&In));
	if (Vect_set_varray_from_db
	    (&In, layer, where_opt->answer, mask_type, 1, varray) == -1) {
	    G_warning(_("Unable to load data from database"));
	}
    }
    else if (cat_opt->answer) {
	if (layer < 1)
	    G_fatal_error(_("'%s' must be > 0 for '%s'"), "layer", "cat");
	varray = Vect_new_varray(Vect_get_num_lines(&In));
	chcat = 1;
	if (Vect_set_varray_from_cat_string
	    (&In, layer, cat_opt->answer, mask_type, 1, varray) == -1) {
	    G_warning(_("Problem loading category values"));
	}
    }
    else {
	chcat = 0;
	varray = NULL;
    }

    /* Create table */
    Fi = Vect_default_field_info(&Out, 1, NULL, GV_1TABLE);
    Vect_map_add_dblink(&Out, 1, NULL, Fi->table, "cat", Fi->database,
			Fi->driver);

    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
	G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
		      Fi->database, Fi->driver);

    sprintf(buf,
	    "create table %s ( from_cat integer, to_cat integer, cost double precision)",
	    Fi->table);

    db_set_string(&sql, buf);
    G_debug(2, db_get_string(&sql));

    if (db_execute_immediate(driver, &sql) != DB_OK) {
	db_close_database_shutdown_driver(driver);
	G_fatal_error(_("Unable to create table: '%s'"), db_get_string(&sql));
    }

    if (db_create_index2(driver, Fi->table, "cat") != DB_OK)
	G_warning(_("Cannot create index"));

    if (db_grant_on_table
	(driver, Fi->table, DB_PRIV_SELECT, DB_GROUP | DB_PUBLIC) != DB_OK)
	G_fatal_error(_("Cannot grant privileges on table <%s>"), Fi->table);

    db_begin_transaction(driver);


    Vect_net_build_graph(&In, mask_type, atoi(field_opt->answer), 0,
			 afcol->answer, abcol->answer, NULL, geo, 0);
    graph = &(In.graph);
    nnodes = dglGet_NodeCount(graph);
    dist = (dglInt32_t **) G_calloc(nnodes + 1, sizeof(dglInt32_t *));
    cats = (int *)G_calloc(nnodes + 1, sizeof(int));	/*id of each node. -1 if not used */

    if (!dist || !cats)
	G_fatal_error(_("Out of memory"));
    for (i = 0; i <= nnodes; i++) {
	dist[i] = (dglInt32_t *) G_calloc(nnodes + 1, sizeof(dglInt32_t));
	if (!dist[i])
	    G_fatal_error(_("Out of memory"));
    }
    neta_allpairs(graph, dist);

    for (i = 1; i <= nnodes; i++)
	cats[i] = -1;

    nlines = Vect_get_num_lines(&In);
    max_cat = 0;
    for (i = 1; i <= nlines; i++) {
	int type = Vect_read_line(&In, Points, Cats, i);
	for (j = 0; j < Cats->n_cats; j++)
	    if (Cats->cat[j] > max_cat)
		max_cat = Cats->cat[j];
	if (type == GV_POINT) {
	    int node;
	    Vect_get_line_nodes(&In, i, &node, NULL);
	    Vect_cat_get(Cats, layer, &cats[node]);
	    if (cats[node] != -1)
		Vect_write_line(&Out, GV_POINT, Points, Cats);
	}

    }
    max_cat++;
    for (i = 1; i <= nnodes; i++)
	if (newpoints_f->answer && cats[i] == -1) {
	    Vect_reset_cats(Cats);
	    Vect_cat_set(Cats, 1, max_cat);
	    cats[i] = max_cat++;
	    neta_add_point_on_node(&In, &Out, i, Cats);
	}
    G_message(_("Writing data into the table..."));
    G_percent_reset();
    for (i = 1; i <= nnodes; i++) {
	G_percent(i, nnodes, 1);
	if (cats[i] != -1)
	    for (j = 1; j <= nnodes; j++)
		if (cats[j] != -1) {
		    sprintf(buf, "insert into %s values (%d, %d, %f)",
			    Fi->table, i, j, dist[i][j] / 1000.0);
		    db_set_string(&sql, buf);
		    G_debug(3, db_get_string(&sql));

		    if (db_execute_immediate(driver, &sql) != DB_OK) {
			db_close_database_shutdown_driver(driver);
			G_fatal_error(_("Cannot insert new record: %s"),
				      db_get_string(&sql));
		    };
		}
    }
    db_commit_transaction(driver);
    db_close_database_shutdown_driver(driver);

    for (i = 0; i <= nnodes; i++)
	G_free(dist[i]);
    G_free(dist);

    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);

    Vect_hist_command(&Out);

    Vect_build(&Out);

    Vect_close(&In);
    Vect_close(&Out);

    exit(EXIT_SUCCESS);
}

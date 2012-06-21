/* PURPOSE:      Parse and validate the input */

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>
#include "iseg.h"

int parse_args(int argc, char *argv[], struct files *files,
	       struct functions *functions)
{
    /* reference: http://grass.osgeo.org/programming7/gislib.html#Command_Line_Parsing */

    struct Option *group, *seeds, *bounds, *output, *method, *threshold;	/* Establish an Option pointer for each option */
    struct Flag *diagonal, *weighted;	/* Establish a Flag pointer for each option */

    /* required parameters */
    group = G_define_standard_option(G_OPT_I_GROUP);	/* TODO: OK to require the user to create a group?  Otherwise later add an either/or option to give just a single raster map... */

    output = G_define_standard_option(G_OPT_R_OUTPUT);

    threshold = G_define_option();
    threshold->key = "threshold";
    threshold->type = TYPE_DOUBLE;
    threshold->required = YES;
    threshold->description = _("Similarity threshold.");

    method = G_define_option();
    method->key = "method";
    method->type = TYPE_STRING;
    method->required = YES;
    method->answer = "region_growing";
    method->options = "region_growing, io_debug, ll_test";	/*io_debug just writes row+col to each output pixel, ll_test for testing linked list data structure */
    method->description = _("Segmentation method.");

    /* optional parameters */

    diagonal = G_define_flag();
    diagonal->key = 'd';
    diagonal->description =
	_("Use 8 neighbors (3x3 neighborhood) instead of the default 4 neighbors for each pixel.");

    weighted = G_define_flag();
    weighted->key = 'w';
    weighted->description =
	_("Weighted input, don't perform the default scaling of input maps.");

    /* Using raster for seeds, Low priority TODO: allow vector points/centroids seed input. */
    seeds = G_define_standard_option(G_OPT_R_INPUT);
    seeds->key = "seeds";
    seeds->type = TYPE_STRING;
    seeds->required = NO;
    seeds->description = _("Optional raster map with starting seeds.");

    /* Polygon constraints. */
    bounds = G_define_standard_option(G_OPT_R_INPUT);
    bounds->key = "bounds";
    bounds->type = TYPE_STRING;
    bounds->required = NO;
    bounds->description =
	_("Optional bounding/constraining raster map, must be integer values, each area will be segmented independent of the others.");
    /*    bounds->description = _("Optional vector map with polygons to bound (constrain) segmentation."); */
    /* TODO: if performing second segmentation, will already have raster map from this module
     * If have preexisting boundaries (landuse, etc) will have vector map
     * Seems we need to have it in raster format for processing, is it OK to have the user run v.to.rast before doing the segmentation?
     * Or for hierarchical segmentation, will it be easier to have the polygons?
     * ....will start with rasters, quickest to implement.... */

    /* TODO input for distance function */

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);


    /* G_debug(1, "For the option <%s> you chose: <%s>",
       group->description, group->answer);

       G_debug(1, "For the option <%s> you chose: <%s>",
       seeds->description, seeds->answer);

       G_debug(1, "For the option <%s> you chose: <%s>",
       output->description, output->answer);

       G_debug(1, "For the option <%s> you chose: <%s>",
       method->description, method->answer);

       G_debug(1, "For the option <%s> you chose: <%s>",
       threshold->description, threshold->answer);

       G_debug(1, "The value of the diagonal flag is: %d", diagonal->answer); */

    /* Validation */

    /* TODO: use checker for any of the data validation steps!? */

    /* ToDo The most important things to check are if the
       input and output raster maps can be opened (non-negative file
       descriptor).  Do this here or in open_files?  */


    /* Check and save parameters */

    files->image_group = group->answer;

    if (G_legal_filename(output->answer) == 1)
	files->out_name = output->answer;	/* name of output raster map */
    else
	G_fatal_error("Invalid output raster name.");

    /* TODO: I'm assuming threshold is already validated as a number.  Is this OK, or is there a better way to cast the input? */
    /* reference r.cost line 313 
       if (sscanf(opt5->answer, "%d", &maxcost) != 1 || maxcost < 0)
       G_fatal_error(_("Inappropriate maximum cost: %d"), maxcost); */
    sscanf(threshold->answer, "%f", &functions->threshold);	/* Note: this gets changed after we know more at beginning of create_isegs() */

    /* segmentation methods:  0 = debug, 1 = region growing */
    /* TODO, instead of string compare, does the Option structure have these already numbered? */

    if (strncmp(method->answer, "io_debug", 5) == 0)
	functions->method = 0;
    else if (strncmp(method->answer, "region_growing", 10) == 0)
	functions->method = 1;
    else if (strncmp(method->answer, "ll_test", 5) == 0)
	functions->method = 2;
    else
	G_fatal_error("Couldn't assign segmentation method.");	/*shouldn't be able to get here */

    G_debug(1, "segmentation method: %d", functions->method);

    if (diagonal->answer == 0) {
	functions->find_pixel_neighbors = &find_four_pixel_neighbors;
	functions->num_pn = 4;
	G_debug(1, "four pixel neighborhood");
    }
    else if (diagonal->answer == 1) {
	functions->find_pixel_neighbors = &find_eight_pixel_neighbors;
	functions->num_pn = 8;
	G_debug(1, "eight (3x3) pixel neighborhood");
    }

    files->weighted = weighted->answer;	/* default/0 for performing the scaling, but selected/1 if user has weighted values so scaling should be skipped. */

    functions->calculate_similarity = &calculate_euclidean_similarity;	/* TODO add user input for this */

    files->seeds = seeds->answer;

    /*todo, check error trapping here, what if nothing is entered?  what if nothing is found? what if in same mapset */
    if (bounds->answer == NULL) {	/*no polygon constraints */
	files->bounds_map = NULL;
    }
    else {			/* polygon constraints given */

	files->bounds_map = bounds->answer;	/*todo, this needs to set files->bounds = NULL if no answer was given to the parameter */
	if ((files->bounds_mapset = G_find_raster(files->bounds_map, "")) == NULL) {	/* TODO, warning here:  parse_args.c:149:27: warning: assignment discards ‘const’ qualifier from pointer target type [enabled by default] */
	    G_fatal_error(_("Segmentation constraint/boundary map not found."));
	}
    }
    /* todo, check raster type, needs to be CELL (integer) */
    /*todo print out what these were before and after */

    /* other data */
    files->nrows = Rast_window_rows();
    files->ncols = Rast_window_cols();

    return 0;
}

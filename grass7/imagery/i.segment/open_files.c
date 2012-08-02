/* PURPOSE:      opening input rasters and creating segmentation files */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include <grass/segment.h>	/* segmentation library */
#include "iseg.h"

int open_files(struct files *files)
{
    struct Ref Ref;		/* group reference list */
    int *in_fd, seeds_fd, bounds_fd, null_check, out_fd, mean_fd;
    int n, s, row, col, srows, scols, inlen, nseg;
    DCELL **inbuf;		/* buffer array, to store lines from each of the imagery group rasters */
    CELL *boundsbuf;
    void *seedsbuf, *ptr;	/* todo. correct data type when allowing any data type? */
    size_t ptrsize;
    RASTER_MAP_TYPE data_type;
    struct FPRange *fp_range;	/* for getting min/max values on each input raster */
    DCELL *min, *max;

    /* confirm output maps can be opened (don't want to do all this work for nothing!) */
    out_fd = Rast_open_new(files->out_name, CELL_TYPE);
    if (out_fd < 0)
	G_fatal_error(_("Could not open output raster for writing segment ID's"));
    else
	Rast_unopen(out_fd);

    if (files->out_band != NULL) {
	mean_fd = Rast_open_new(files->out_band, DCELL_TYPE);
	if (mean_fd < 0)
	    G_fatal_error(_("Could not open output raster for writing mean segment values"));
	else
	    Rast_unopen(mean_fd);
    }

    /*allocate memory for flags */
    files->null_flag = flag_create(files->nrows, files->ncols);
    files->candidate_flag = flag_create(files->nrows, files->ncols);
    if (files->bounds_map != NULL)
	files->orig_null_flag = flag_create(files->nrows, files->ncols);
    if (files->seeds_map != NULL)
	files->seeds_flag = flag_create(files->nrows, files->ncols);

    /* references for segmentation library: i.cost r.watershed/seg and http://grass.osgeo.org/programming7/segmentlib.html */

    /* ****** open the input rasters ******* */

    /* Note: I confirmed, the API does not check this. */
    G_debug(1, "Checking image group...");
    if (!I_get_group_ref(files->image_group, &Ref))
	G_fatal_error(_("Unable to read REF file for group <%s>"),
		      files->image_group);

    if (Ref.nfiles <= 0)
	G_fatal_error(_("Group <%s> contains no raster maps"),
		      files->image_group);

    /* Read Imagery Group */

    in_fd = G_malloc(Ref.nfiles * sizeof(int));
    inbuf = (DCELL **) G_malloc(Ref.nfiles * sizeof(DCELL *));
    fp_range = G_malloc(Ref.nfiles * sizeof(struct FPRange));
    min = G_malloc(Ref.nfiles * sizeof(DCELL));
    max = G_malloc(Ref.nfiles * sizeof(DCELL));

    G_debug(1, "Opening input rasters...");
    for (n = 0; n < Ref.nfiles; n++) {
	inbuf[n] = Rast_allocate_d_buf();
	in_fd[n] = Rast_open_old(Ref.file[n].name, Ref.file[n].mapset);
	if (in_fd[n] < 0)
	    G_fatal_error("Error opening %s@%s", Ref.file[n].name,
			  Ref.file[n].mapset);
    }

    /* open seeds raster */
    if (files->seeds_map != NULL) {
	seeds_fd = Rast_open_old(files->seeds_map, "");
	data_type = Rast_get_map_type(seeds_fd);
	seedsbuf = Rast_allocate_buf(data_type);
	ptrsize = Rast_cell_size(data_type);
    }

    /* Get min/max values of each input raster for scaling */

    if (files->weighted == FALSE) {	/*default, we will scale */
	for (n = 0; n < Ref.nfiles; n++) {
	    if (Rast_read_fp_range(Ref.file[n].name, Ref.file[n].mapset, &fp_range[n]) != 1)	/* returns -1 on error, 2 on empty range, quiting either way. */
		G_fatal_error(_("No min/max found in raster map <%s>"),
			      Ref.file[n].name);
	    Rast_get_fp_range_min_max(&(fp_range[n]), &min[n], &max[n]);
	}
	G_debug(1, "scaling, for first layer, min: %f, max: %f",
		min[0], max[0]);
    }

    /* ********** find out file segmentation size ************ */
    G_debug(1, "Calculate temp file sizes...");

    files->nbands = Ref.nfiles;

    /* size of each element to be stored */

    inlen = sizeof(double) * Ref.nfiles;

    /* when fine tuning, should be a power of 2 and not larger than 256 for speed reasons */
    srows = 64;
    scols = 64;

    /* TODO: make calculations for this, check i.cost and i.watershed */
    nseg = 16;			// 1000;         //16;


    /* ******* create temporary segmentation files ********* */
    G_debug(1, "Getting temporary file names...");
    /* Initalize access to database and create temporary files */

    G_debug(1, "Image size:  %d rows, %d cols", files->nrows, files->ncols);
    G_debug(1, "Segmented to tiles with size:  %d rows, %d cols", srows,
	    scols);
    G_debug(1, "Data element size, in: %d", inlen);
    G_debug(1, "number of segments to have in memory: %d", nseg);

    if (segment_open
	(&files->bands_seg, G_tempfile(), files->nrows, files->ncols, srows,
	 scols, inlen, nseg) != TRUE)
	G_fatal_error("Unable to create input temporary files");

    /* ******* remaining memory allocation ********* */

    files->bands_val = (double *)G_malloc(inlen);
    files->second_val = (double *)G_malloc(inlen);

    if (segment_open
	(&files->iseg_seg, G_tempfile(), files->nrows, files->ncols, srows,
	 scols, sizeof(int), nseg) != TRUE)
	G_fatal_error(_("Unable to allocate memory for initial segment ID's"));
    /* NOTE: SEGMENT file should be initialized to zeros for all data. TODO double check this. */

    /* ********  load input bands to segment structure and fill initial seg ID's ******** */
    G_debug(1, "Reading input rasters into segmentation data files...");
    s = 0;			/* initial segment ID will be 1 */

    for (row = 0; row < files->nrows; row++) {
	for (n = 0; n < Ref.nfiles; n++) {
	    Rast_get_d_row(in_fd[n], inbuf[n], row);
	}
	if (files->seeds_map != NULL) {
	    Rast_get_row(seeds_fd, seedsbuf, row, data_type);
	    ptr = seedsbuf;
	}
	for (col = 0; col < files->ncols; col++) {
	    null_check = 1;	/*Assume there is data */
	    for (n = 0; n < Ref.nfiles; n++) {
		if (Rast_is_d_null_value(&inbuf[n][col]))
		    null_check = -1;
		if (files->weighted == TRUE)
		    files->bands_val[n] = inbuf[n][col];	/*unscaled */
		else
		    files->bands_val[n] = (inbuf[n][col] - min[n]) / (max[n] - min[n]);	/*scaled version */
	    }
	    segment_put(&files->bands_seg, (void *)files->bands_val, row, col);	/* store input bands */

	    if (null_check != -1) {	/*good pixel */
		FLAG_UNSET(files->null_flag, row, col);	/*flag */
		if (files->seeds_map != NULL) {
		    //G_message("Rast_is_null_value(&seedsbuf[col], data_type) = %d", Rast_is_null_value(&seedsbuf[col], data_type));
		    // if (Rast_is_null_value(&seedsbuf[col], data_type) == TRUE)  //switched to ptr navigation...  maybe I could have done &(seedsbuf[col]) ??

		    if (Rast_is_null_value(ptr, data_type) == TRUE) {	/* TODO, compiler warnings:
									 * open_files.c:182:37: warning: dereferencing ‘void *’ pointer [enabled by default]
									 * open_files.c:182:27: warning: taking address of expression of type ‘void’ [enabled by default] */
			// not needed, initialized to zero.  files->iseg[row][col] = 0; /* place holder... todo, Markus, is this OK to leave out?  Or better to set it, since this is just done once... */
			FLAG_UNSET(files->seeds_flag, row, col);	//todo shouldn't need to this, flag is initialized to zero?
		    }
		    else {
			s++;	/* sequentially number each seed pixel with its own segment ID */
			FLAG_SET(files->seeds_flag, row, col);	//todo might not need this... just use the zero as seg ID?  If go this route, need to enforce constraints are positive integers.
			segment_put(&files->iseg_seg, &s, row, col);
			G_message("set seed for row: %d, col: %d", row, col);

		    }
		    ptr = G_incr_void_ptr(ptr, ptrsize);
		}
		else {		/* no seeds provided */
		    s++;	/* sequentially number all pixels with their own segment ID */
		    segment_put(&files->iseg_seg, &s, row, col);	/*starting segment number */
		}
	    }
	    else {		/*don't use this pixel */
		//files->iseg[row][col] = -1;   /* place holder...TODO this could be a conflict if constraints included a -1 ??? wrote that awhile ago...still true???*/
		//TODO, do we need a -1 here?  Or just leave as the zero it was initialized with?  Need a -1 if we don't use the NULL_FLAG...
		FLAG_SET(files->null_flag, row, col);	/*flag */
	    }
	}
    }

    /* number of initial segments, will decrement when merge */
    files->nsegs = s;

    /* bounds/constraints */
    if (files->bounds_map != NULL) {
	if (segment_open
	    (&files->bounds_seg, G_tempfile(), files->nrows, files->ncols,
	     srows, scols, sizeof(int), nseg) != TRUE)
	    G_fatal_error("Unable to create bounds temporary files");

	boundsbuf = Rast_allocate_c_buf();
	bounds_fd = Rast_open_old(files->bounds_map, files->bounds_mapset);

	for (row = 0; row < files->nrows; row++) {
	    Rast_get_c_row(bounds_fd, boundsbuf, row);
	    for (col = 0; col < files->ncols; col++) {
		files->bounds_val = boundsbuf[col];
		segment_put(&files->bounds_seg, &files->bounds_val, row, col);
		if (Rast_is_c_null_value(&boundsbuf[col]) == TRUE) {
		    FLAG_SET(files->null_flag, row, col);
		}
	    }
	}
	Rast_close(bounds_fd);
	G_free(boundsbuf);

	/* keep original copy of null flag if we have boundary constraints */
	for (row = 0; row < files->nrows; row++) {
	    for (col = 0; col < files->ncols; col++) {
		if (FLAG_GET(files->null_flag, row, col))
		    FLAG_SET(files->orig_null_flag, row, col);
		else		/* todo polish, flags are initialized to zero... could just skip this else? */
		    FLAG_UNSET(files->orig_null_flag, row, col);
	    }
	}
    }				/* end: if (files->bounds_map != NULL) */
    else {
	G_debug(1, "no boundary constraint supplied.");
    }

    /* other info */
    files->candidate_count = 0;	/* counter for remaining candidate pixels */

    /* translate seeds to unique segments TODO MM mentioned it here... */
    /* todo decide if we need to take the seeds value, and use it to start the segments... or if each pixel starts a unique segment. */

    /* linked list memory management linkm */
    link_set_chunk_size(100);	/* TODO polish: fine tune this number */

    files->token = link_init(sizeof(struct pixels));

    /* Free memory */

    for (n = 0; n < Ref.nfiles; n++) {
	G_free(inbuf[n]);
	Rast_close(in_fd[n]);
    }

    if (files->seeds_map != NULL) {
	Rast_close(seeds_fd);
	G_free(seedsbuf);
    }

    G_free(inbuf);
    G_free(in_fd);
    G_free(fp_range);
    G_free(min);
    G_free(max);

    return TRUE;
}

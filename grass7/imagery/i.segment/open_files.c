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
    int *in_fd;
    int n, row, col, srows, scols, inlen, outlen, nseg;
    DCELL **inbuf;		/* buffer array, to store lines from each of the imagery group rasters */
    struct FPRange *fp_range;	/* for getting min/max values on each input raster */
    DCELL *min, *max;

    G_debug(1, "Checking image group...");
    /* references: i.cost r.watershed/seg and http://grass.osgeo.org/programming7/segmentlib.html */

    /* ****** open the input rasters ******* */

    /* TODO, this is from i.smap/openfiles.c  lines 17-23 checked if subgroup had maps, does API handles the checks? */

    if (!I_get_group_ref(files->image_group, &Ref))
	G_fatal_error(_("Unable to read REF file for group <%s>"),
		      files->image_group);

    if (Ref.nfiles <= 0)
	G_fatal_error(_("Group <%s> contains no raster maps"),
		      files->image_group);

    /* Read Imagery Group */

    in_fd = G_malloc(Ref.nfiles * sizeof(int));
    inbuf = (DCELL **) G_malloc(Ref.nfiles * sizeof(DCELL *));
    fp_range = G_malloc(Ref.nfiles * sizeof(struct FPRange));	/* TODO, is this correct memory allocation for these three? */
    min = G_malloc(Ref.nfiles * sizeof(DCELL));
    max = G_malloc(Ref.nfiles * sizeof(DCELL));

    G_debug(1, "Opening input rasters...");
    for (n = 0; n < Ref.nfiles; n++) {
	inbuf[n] = Rast_allocate_d_buf();
	in_fd[n] = Rast_open_old(Ref.file[n].name, Ref.file[n].mapset);
    }

    /* Get min/max values of each input raster for scaling */

    if (files->weighted == 0) {	/*default, we will scale */
	for (n = 0; n < Ref.nfiles; n++) {
	    if (Rast_read_fp_range
		(Ref.file[n].name, Ref.file[n].mapset, &fp_range[n]) < 0)
		G_fatal_error(_("Unable to read fp range for raster map <%s>"), Ref.file[n].name);	/* TODO, still should close files? */
	    Rast_get_fp_range_min_max(&(fp_range[n]), &min[n], &max[n]);
	}
	G_verbose_message("scaling, for first layer, min: %f, max: %f",
			  min[0], max[0]);
    }

    /* ********** find out file segmentation size ************ */
    G_debug(1, "Calculate temp file sizes...");

    files->nbands = Ref.nfiles;

    /* size of each element to be stored */

    inlen = sizeof(double) * Ref.nfiles;
    outlen = sizeof(int) * 2;	/* change in write_output.c if this value changes TODO: better to save this in the files data structure? */

    /*TODO: i.cost and i.watershed take different approaches...hardcode for now */
    /* when fine tuning, should be a power of 2 and not larger than 256 for speed reasons */
    srows = 64;
    scols = 64;

    /* TODO: make calculations for this */
    nseg = 8;


    /* ******* create temporary segmentation files ********* */
    G_debug(1, "Getting temporary file names...");
    /* Initalize access to database and create temporary files */

    G_debug(1, "Image size:  %d rows, %d cols", files->nrows, files->ncols);
    G_debug(1, "Segmented to tiles with size:  %d rows, %d cols", srows,
	    scols);
    G_debug(1, "Data element size, in: %d , out: %d ", inlen, outlen);
    G_debug(1, "number of segments to have in memory: %d", nseg);

    /* size: reading all input bands as DCELL  TODO: could consider checking input to see if it is all FCELL or CELL, could reduce memory requirements. */

    if (segment_open
	(&files->bands_seg, G_tempfile(), files->nrows, files->ncols, srows,
	 scols, inlen, nseg) != 1)
	G_fatal_error("Unable to create input temporary files");

    G_debug(1, "finished segment_open(...bands_seg...)");

    /* TODO: signed integer gives a 2 billion segment limit, depending on how the initialization is done, this means 2 billion max input pixels. */
    if (segment_open
	(&files->out_seg, G_tempfile(), files->nrows, files->ncols, srows,
	 scols, outlen, nseg) != 1)
	G_fatal_error("Unable to create output temporary files");

    if (segment_open(&files->no_check, G_tempfile(), files->nrows, files->ncols, srows, scols, sizeof(int), nseg) != 1)	/* todo could make this smaller ? just need 0 or 1 */
	G_fatal_error("Unable to create flag temporary files");

    /* load input bands to segment structure */
    G_debug(1, "Reading input rasters into segmentation data files...");

    files->bands_val = (double *)G_malloc(inlen);
    files->second_val = (double *)G_malloc(inlen);

    for (row = 0; row < files->nrows; row++) {
	for (n = 0; n < Ref.nfiles; n++) {
	    Rast_get_d_row(in_fd[n], inbuf[n], row);
	}
	for (col = 0; col < files->ncols; col++) {
	    for (n = 0; n < Ref.nfiles; n++) {
		if (files->weighted == 1)
		    files->bands_val[n] = inbuf[n][col];	/*unscaled */
		else
		    files->bands_val[n] = (inbuf[n][col] - min[n]) / (max[n] - min[n]);	/*scaled version */
	    }
	    segment_put(&files->bands_seg, (void *)files->bands_val, row,
			col);
	}
    }

    /* Initialize output segmentation file */

    /* TODO: Need to carry MASK over to the output file as well.  Need an additional flag that is set, during input read?  */

    G_debug(1, "G_malloc size: = <%d>", (int)(2 * sizeof(int)));
    files->out_val = (int *)G_malloc(2 * sizeof(int));

    n = 1;
    for (row = 0; row < files->nrows; row++) {
	for (col = 0; col < files->ncols; col++) {
	    files->out_val[0] = n;	/*starting segment number TODO: for seeds this will be different */
	    files->out_val[1] = 0;	/*flag */
	    segment_put(&files->out_seg, (void *)files->out_val, row, col);
	    n++;		/* sequentially number all pixels with their own segment ID */
	}
    }

    files->candidate_count = 0;	/* counter for remaining candidate pixels */


    /* linked list memory management linkm */
    link_set_chunk_size(20);	/* TODO: fine tune this number */

    files->token = link_init(sizeof(struct pixels));

    /* Free memory */

    for (n = 0; n < Ref.nfiles; n++) {
	Rast_close(in_fd[n]);
    }

    G_free(inbuf);
    G_free(in_fd);
    G_free(fp_range);
    G_free(min);
    G_free(max);

    /* Need to clean up anything else? */

    return 0;
}

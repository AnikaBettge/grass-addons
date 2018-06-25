
/****************************************************************************
 *
 * MODULE:       r.accumulate
 *
 * AUTHOR(S):    Huidae Cho <grass4u gmail.com>
 *
 * PURPOSE:      Calculates weighted flow accumulation and delineates stream
 *               networks using a flow direction map.
 *
 * COPYRIGHT:    (C) 2018 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#define _MAIN_C_

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "global.h"

#define DIR_UNKNOWN 0
#define DIR_DEG 1
#define DIR_DEG45 2

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct
    {
        struct Option *dir;
        struct Option *format;
        struct Option *weight;
        struct Option *accum;
        struct Option *thresh;
        struct Option *stream;
    } opt;
    struct
    {
        struct Flag *neg;
    } flag;
    char *desc;
    char *dir_name, *weight_name, *accum_name, *stream_name;
    int dir_fd, weight_fd, accum_fd;
    double dir_format, thresh;
    struct Range dir_range;
    CELL dir_min, dir_max;
    char neg;
    char **done;
    struct cell_map dir_buf;
    struct raster_map weight_buf, accum_buf;
    int rows, cols, row, col;
    struct History hist;
    struct Map_info Map;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("raster"));
    G_add_keyword(_("hydrology"));
    module->description =
        _("Calculates weighted flow accumulation and delineates stream networks using a flow direction map.");

    opt.dir = G_define_standard_option(G_OPT_R_INPUT);
    opt.dir->key = "direction";
    opt.dir->description = _("Name of input direction map");

    opt.format = G_define_option();
    opt.format->type = TYPE_STRING;
    opt.format->key = "format";
    opt.format->label = _("Format of input direction map");
    opt.format->required = YES;
    opt.format->options = "auto,degree,45degree";
    opt.format->answer = "auto";
    G_asprintf(&desc, "auto;%s;degree;%s;45degree;%s",
               _("auto-detect direction format"),
               _("degrees CCW from East"),
               _("degrees CCW from East divided by 45 (e.g. r.watershed directions)"));
    opt.format->descriptions = desc;

    opt.weight = G_define_standard_option(G_OPT_R_INPUT);
    opt.weight->key = "weight";
    opt.weight->required = NO;
    opt.weight->description = _("Name of input flow weight map");

    opt.accum = G_define_standard_option(G_OPT_R_OUTPUT);
    opt.accum->key = "accumulation";
    opt.accum->required = NO;
    opt.accum->type = TYPE_STRING;
    opt.accum->description =
        _("Name for output weighted flow accumulation map");

    opt.thresh = G_define_option();
    opt.thresh->key = "threshold";
    opt.thresh->type = TYPE_DOUBLE;
    opt.thresh->label = _("Minimum flow accumulation for streams");

    opt.stream = G_define_standard_option(G_OPT_V_OUTPUT);
    opt.stream->key = "stream";
    opt.stream->required = NO;
    opt.stream->description = _("Name for output stream vector map");

    flag.neg = G_define_flag();
    flag.neg->key = 'n';
    flag.neg->label =
        _("Use negative flow accumulation for likely underestimates");

    /* weighting doesn't support negative accumulation because weights
     * themselves can be negative */
    G_option_exclusive(opt.weight, flag.neg, NULL);
    G_option_required(opt.accum, opt.stream, NULL);
    G_option_collective(opt.thresh, opt.stream, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    dir_name = opt.dir->answer;
    weight_name = opt.weight->answer;
    accum_name = opt.accum->answer;
    stream_name = opt.stream->answer;

    dir_fd = Rast_open_old(dir_name, "");
    if (Rast_get_map_type(dir_fd) != CELL_TYPE)
        G_fatal_error(_("Invalid directions map <%s>"), dir_name);

    /* adapted from r.path */
    if (Rast_read_range(dir_name, "", &dir_range) < 0)
        G_fatal_error(_("Unable to read range file"));
    Rast_get_range_min_max(&dir_range, &dir_min, &dir_max);
    if (dir_max <= 0)
        G_fatal_error(_("Invalid directions map <%s>"), dir_name);

    dir_format = DIR_UNKNOWN;
    if (strcmp(opt.format->answer, "degree") == 0) {
        if (dir_max > 360)
            G_fatal_error(_("Directional degrees can not be > 360"));
        dir_format = DIR_DEG;
    }
    else if (strcmp(opt.format->answer, "45degree") == 0) {
        if (dir_max > 8)
            G_fatal_error(_("Directional degrees divided by 45 can not be > 8"));
        dir_format = DIR_DEG45;
    }
    else if (strcmp(opt.format->answer, "auto") == 0) {
        if (dir_max <= 8) {
            dir_format = DIR_DEG45;
            G_important_message(_("Input direction format assumed to be degrees CCW from East divided by 45"));
        }
        else if (dir_max <= 360) {
            dir_format = DIR_DEG;
            G_important_message(_("Input direction format assumed to be degrees CCW from East"));
        }
        else
            G_fatal_error(_("Unable to detect format of input direction map <%s>"),
                          dir_name);
    }
    if (dir_format == DIR_UNKNOWN)
        G_fatal_error(_("Invalid directions format '%s'"),
                      opt.format->answer);
    /* end of r.path */

    thresh = opt.thresh->answer ? atof(opt.thresh->answer) : 0.0;
    neg = flag.neg->answer;

    rows = Rast_window_rows();
    cols = Rast_window_cols();

    /* initialize the done array and read the direction map */
    done = G_malloc(rows * sizeof(char *));
    dir_buf.rows = rows;
    dir_buf.cols = cols;
    dir_buf.c = G_malloc(rows * sizeof(CELL *));
    for (row = 0; row < rows; row++) {
        done[row] = G_calloc(cols, 1);
        dir_buf.c[row] = Rast_allocate_c_buf();
        Rast_get_c_row(dir_fd, dir_buf.c[row], row);
        if (dir_format == DIR_DEG) {
            for (col = 0; col < cols; col++)
                dir_buf.c[row][col] /= 45.0;
        }
    }
    Rast_close(dir_fd);

    /* optionally, read a weight map */
    if (weight_name) {
        weight_fd = Rast_open_old(weight_name, "");
        accum_buf.type = weight_buf.type = Rast_get_map_type(weight_fd);
        weight_buf.rows = rows;
        weight_buf.cols = cols;
        weight_buf.map.v = (void **)G_malloc(rows * sizeof(void *));
        for (row = 0; row < rows; row++) {
            weight_buf.map.v[row] =
                (void *)Rast_allocate_buf(weight_buf.type);
            Rast_get_row(weight_fd, weight_buf.map.v[row], row,
                         weight_buf.type);
        }
        Rast_close(weight_fd);
    }
    else {
        weight_fd = -1;
        weight_buf.map.v = NULL;
        accum_buf.type = CELL_TYPE;
    }

    /* create accumulation buffer */
    accum_buf.rows = rows;
    accum_buf.cols = cols;
    accum_buf.map.v = (void **)G_malloc(rows * sizeof(void *));
    for (row = 0; row < rows; row++)
        accum_buf.map.v[row] = (void *)Rast_allocate_buf(accum_buf.type);

    /* create a new accumulation map if requested */
    accum_fd = accum_name ? Rast_open_new(accum_name, accum_buf.type) : -1;

    /* accumulate flows */
    accumulate(&dir_buf, &weight_buf, &accum_buf, done, neg);

    /* write out buffer to the accumulatoin map */
    if (accum_fd >= 0) {
        for (row = 0; row < rows; row++)
            Rast_put_row(accum_fd, accum_buf.map.v[row], accum_buf.type);
        Rast_close(accum_fd);

        /* write history */
        Rast_put_cell_title(accum_name,
                            weight_name ? "Weighted flow accumulation" :
                            (neg ?
                             "Flow accumulation with likely underestimates" :
                             "Flow accumulation"));
        Rast_short_history(accum_name, "raster", &hist);
        Rast_command_history(&hist);
        Rast_write_history(accum_name, &hist);
    }

    /* free buffer memory */
    for (row = 0; row < rows; row++) {
        G_free(done[row]);
        if (weight_fd >= 0)
            G_free(weight_buf.map.v[row]);
    }
    G_free(done);
    if (weight_fd >= 0)
        G_free(weight_buf.map.v);

    /* delineate stream networks */
    if (stream_name) {
        if (Vect_open_new(&Map, stream_name, 0) < 0)
            G_fatal_error(_("Unable to create vector map <%s>"), stream_name);
        Vect_set_map_name(&Map, "Stream network");
        Vect_hist_command(&Map);

        delineate_streams(&Map, thresh, &dir_buf, &accum_buf);

        if (!Vect_build(&Map))
            G_warning(_("Unable to build topology for vector map <%s>"),
                      stream_name);
        Vect_close(&Map);
    }

    /* free buffer memory */
    for (row = 0; row < rows; row++) {
        G_free(dir_buf.c[row]);
        G_free(accum_buf.map.v[row]);
    }
    G_free(dir_buf.c);
    G_free(accum_buf.map.v);

    exit(EXIT_SUCCESS);
}

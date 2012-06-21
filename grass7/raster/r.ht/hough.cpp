#include "houghtransform.h"

#include"linesegmentsextractor.h"
#include "matrix.h"

extern "C" {
namespace grass {
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/gmath.h>
}
}

using grass::DCELL;
using grass::CELL;
using grass::G__calloc;
using grass::Cell_head;
using grass::Map_info;

using grass::Vect_new_cats_struct;
using grass::Vect_new_line_struct;

using grass::Rast_allocate_d_input_buf;
using grass::Rast_open_old;
using grass::Rast_get_row;
using grass::Rast_close;

using grass::Rast_window_rows;
using grass::Rast_window_cols;
using grass::Rast_allocate_d_buf;
using grass::Rast_open_fp_new;
using grass::Rast_put_d_row;
using grass::Rast_get_cellhd;

using grass::G_gettext;
using grass::G_fatal_error;
using grass::G_debug;
using grass::G_free;

using grass::Colors;
using grass::FPRange; // FIXME: DCELL/CELL
using grass::G_mapset;

/** Loads map into memory.

  \param[out] mat map in a matrix (row order), field have to be allocated
  */
void read_raster_map(const char *name, const char *mapset, int nrows,
                            int ncols, Matrix& mat)
{

    int r, c;

    int map_fd;

    DCELL *row_buffer;

    DCELL cell_value;

    row_buffer = Rast_allocate_d_input_buf();

    /* load map */
    map_fd = Rast_open_old(name, mapset);
    if (map_fd < 0) {
        G_fatal_error(_("Error opening first raster map <%s>"), name);
    }

    G_debug(1, "fd %d %s %s", map_fd, name, mapset);

    //    if ((first_map_R_type =
    //         Rast_map_type(templName, mapset)) < 0)
    //        G_fatal_error(_("Error getting first raster map type"));

    for (r = 0; r < nrows; r++) {
        Rast_get_row(map_fd, row_buffer, r, DCELL_TYPE);

        for (c = 0; c < ncols; c++) {
            cell_value = row_buffer[c];
            if (!Rast_is_d_null_value(&cell_value))
                mat(r, c) = cell_value;
            else
                mat(r, c) = 0.0;
        }
    }
    G_free(row_buffer);

    Rast_close(map_fd);
}

void apply_hough_colors_to_map(const char *name)
{
    struct Colors colors;
    struct FPRange range;
    DCELL min, max;

    Rast_read_fp_range(name, G_mapset(), &range);
    Rast_get_fp_range_min_max(&range, &min, &max);
    Rast_make_grey_scale_colors(&colors, min, max);
    Rast_write_colors(name, G_mapset(), &colors);
}

void create_raster_map(const char *name, struct Cell_head *window, const Matrix& mat)
{
    struct Cell_head original_window;
    DCELL *cell_real;
    int rows, cols; /* number of rows and columns */
    long totsize; /* total number of data points */ // FIXME: make clear the size_t usage
    int realfd;

    /* get the rows and columns in the current window */
    rows = mat.rows();
    cols = mat.columns();
    totsize = rows * cols;

    G_get_set_window(&original_window);

    window->bottom = 0;
    window->top = rows;
    window->cols = cols;
    window->east = cols;
    window->north = rows;
    window->ns_res = 1;
    window->rows = rows;
    window->south = 0;
    window->west = 0;
    window->ew_res = 1;
    window->tb_res = 1;

    Rast_set_window(window);

    /* allocate the space for one row of cell map data */
    cell_real = Rast_allocate_d_buf();

    /* open the output cell maps */
    realfd = Rast_open_fp_new(name);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cell_real[j] = mat(i, j);
        }
        Rast_put_d_row(realfd, cell_real);
    }

    Rast_close(realfd);
    G_free(cell_real);

    Rast_set_window(&original_window);
}

/**
  \param cellhd raster map header, used for converting rows/cols to easting/northing
  */
void create_vector_map(const char * name, const SegmentList& segments,
                       const Cell_head* cellhd)
{
    struct Map_info Map;
    Vect_open_new(&Map, name, 0);

    struct grass::line_cats *Cats;
    Cats = Vect_new_cats_struct();


    for (size_t i = 0; i < segments.size(); ++i)
    {
        const Segment& seg = segments[i];

        struct grass::line_pnts *points = Vect_new_line_struct(); // FIXME: some destroy

        double y1 = Rast_row_to_northing(seg.first.first, cellhd);
        double x1 = Rast_col_to_easting(seg.first.second, cellhd);
        double y2 = Rast_row_to_northing(seg.second.first, cellhd);
        double x2 = Rast_col_to_easting(seg.second.second, cellhd);

        Vect_cat_set(Cats, 1, i+1); // cat is segment number (counting from one)

        Vect_append_point(points, x1, y1, 0);
        Vect_append_point(points, x2, y2, 0);

        Vect_write_line(&Map, GV_LINE, points, Cats);
    }

    Vect_build(&Map);
    Vect_close(&Map);
}

void extract_line_segments(const Matrix &I,
                           const HoughTransform::Peaks& peaks,
                           const HoughTransform::TracebackMap& houghMap,
                           ExtractParametres extractParametres,
                           SegmentList& segments)
{
    for (size_t i = 0; i < peaks.size(); ++i)
    {
        const HoughTransform::Peak& peak = peaks[i];

        double theta = peak.coordinates.second;

        HoughTransform::TracebackMap::const_iterator coordsIt =
                houghMap.find(peak.coordinates);
        if (coordsIt != houghMap.end())
        {
            const HoughTransform::CoordinatesList& lineCoordinates = coordsIt->second;

            LineSegmentsExtractor extractor(I, extractParametres);

            extractor.extract(lineCoordinates, (theta-90)/180*M_PI, segments);
        }
        else
        {
            // logic error
        }
    }
}

void hough_peaks(HoughParametres houghParametres,
                 ExtractParametres extractParametres,
                 const char *name, const char *mapset, size_t nrows, size_t ncols,
                 const char *anglesMapName,
                 const char *houghImageName, const char *result)
{
    Matrix I(nrows, ncols);
    read_raster_map(name, mapset, nrows, ncols, I);

    HoughTransform hough(I, houghParametres);

    if (anglesMapName != NULL)
    {
        Matrix angles(nrows, ncols);
        read_raster_map(anglesMapName, mapset, nrows, ncols, angles);
        hough.compute(angles);
    }
    else
    {
        hough.compute();
    }

    hough.findPeaks();

    if (houghImageName != NULL)
    {
        struct Cell_head window;
        Rast_get_cellhd(name, "", &window);
        create_raster_map(houghImageName, &window, hough.getHoughMatrix());
        apply_hough_colors_to_map(houghImageName);
    }

    const HoughTransform::Peaks& peaks = hough.getPeaks();
    const HoughTransform::TracebackMap& houghMap = hough.getHoughMap();
    SegmentList segments;

    extract_line_segments(I, peaks, houghMap, extractParametres, segments);

    Cell_head cellhd;
    Rast_get_window(&cellhd);
    create_vector_map(result, segments, &cellhd);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"


/*--------------------------------------------------------
  HISTOGRAM ANALYSIS
  Define un factor de escala = hist_n/100 con objeto
  de dividir el entero 1 por 100/hist_n partes y
  aumentar la precision.

  Afecta al almacenamiento en el histograma pero
  modifica el calculo de quantiles y momentos.
 --------------------------------------------------------*/

/* Global variable
   allow use as parameters of command line */
int hist_n   = 100;  /* interval 100/hist_n */

void hist_put(double t, int hist[])
{
    int i;

    /* scale factor */
    i = (int)(t * ((double)hist_n/100.));

    if (i < 1) i = 1;
    if (i > hist_n) i = hist_n;

    hist[i - 1] += 1;
}

/* Media real no del histograma */
double mean(int hist[])
{
    int i, total;
    double mean;

    total = 0;
    mean  = 0.;
    for( i = 0; i < hist_n; i++ )
    {
        total += hist[i];
        mean += (double)(i * hist[i]);
    }
    mean /= (double)total;

    /* remove scale factor */
    return (mean / ((double)hist_n/100.));
}

double moment(int n, int hist[])
{
    int i, j, total;
    double value, hmean, temp;

    total = 0;
    hmean  = 0.;
    for( i = 0; i < hist_n; i++ )
    {
        total += hist[i];
        hmean += (double)(i * hist[i]);
    }
    hmean /= (double)total;

    value = 0.;
    for( i = 0; i < hist_n; i++ )
    {
        temp = 1.;
        for( j = 0; j < n; j++ ) temp *= (i - hmean);
        value += temp * (double)hist[i]/(double)total;
    }

    /* remove scale factor */
    for( j = 0; j < n; j++ ) value /= ((double)hist_n/100.);
    return value;
}

double quantile(double q, int hist[])
{
    int i, total;
    double value, qmax, qmin;

    total = 0;
    for( i = 0; i < hist_n; i++ )
    {
        total += hist[i];
    }

    qmax = 1.;
    for( i = hist_n - 1; i >= 0; i-- )
    {
        qmin = qmax - (double)hist[i]/(double)total;
        if( q >= qmin )
        {
            value = (q - qmin) / (qmax - qmin) + (i - 1);
            break;
        }
        qmax = qmin;
    }

    /* remove scale factor */
    return (value / ((double)hist_n/100.));
}



/*--------------------------------------------------------
    FILTER HOLES OF CLOUDS
    This a >=50% filter of 3x3
    if >= 50% vecinos cloud then pixel set to cloud
 --------------------------------------------------------*/

#define UNDEFINED   -1

int pval(void * rast, int i)
{
    void * ptr = (void *)((CELL *)rast + i);
    if( G_is_c_null_value(ptr) )
        return 0;
    else
        return (int)((CELL *) rast)[i];
}

void filter_holes(int verbose, Gfile * out)
{
    int row, col, nrows, ncols;
    char *mapset;

    void *arast, *brast, *crast;
    int  i, pixel[9], cold, warm, shadow, nulo, lim;

    Gfile tmp;

    nrows = G_window_rows();
    ncols = G_window_cols();

    if( nrows < 3 || ncols < 3 )
        return;

    /* Open to read */
    mapset = G_find_cell2(out->name, "");
    if (mapset == NULL)
        G_fatal_error("cell file [%s] not found", out->name);
    arast = G_allocate_raster_buf(CELL_TYPE);
    brast = G_allocate_raster_buf(CELL_TYPE);
    crast = G_allocate_raster_buf(CELL_TYPE);
    if ((out->fd = G_open_cell_old(out->name, mapset)) < 0)
        G_fatal_error("Cannot open cell file [%s]", out->name);

    /* Open to write */
    sprintf(tmp.name, "_%d.BBB", getpid()) ;
    tmp.rast = G_allocate_raster_buf(CELL_TYPE);
    if ((tmp.fd = G_open_raster_new(tmp.name, CELL_TYPE)) < 0)
        G_fatal_error(_("Could not open <%s>"), tmp.name);

    fprintf(stdout, "Filling cloud holes ... \n");


    for (row = 0; row < nrows; row++)
    {
        if (verbose)
        {
            G_percent(row, nrows, 2);
        }
        /* Read row values */
        if (row != 0)
        {
            if (G_get_c_raster_row(out->fd, arast, row - 1) < 0)
                G_fatal_error(_("Could not read from <%s>"), out->name);
        }
        if (G_get_c_raster_row(out->fd, brast, row) < 0)
        {
            G_fatal_error(_("Could not read from <%s>"), out->name);
        }
        if (row != (nrows - 1))
        {
            if (G_get_c_raster_row(out->fd, crast, row + 1) < 0)
                G_fatal_error(_("Could not read from <%s>"), out->name);
        }
        /* Analysis of all pixels */
        for (col = 0; col < ncols; col++)
        {
            pixel[0] = pval(brast, col);
            if( pixel[0] == 0 )
            {
                if( row == 0 )
                {
                    pixel[1] = -1;
                    pixel[2] = -1;
                    pixel[3] = -1;
                    if( col == 0 )
                    {
                        pixel[4] = -1;
                        pixel[5] = pval(brast, col + 1);
                        pixel[6] = -1;
                        pixel[7] = pval(crast, col);
                        pixel[8] = pval(crast, col + 1);
                    }
                    else if( col != (ncols - 1) )
                    {
                        pixel[4] = pval(brast, col - 1);
                        pixel[5] = pval(brast, col + 1);
                        pixel[6] = pval(crast, col - 1);
                        pixel[7] = pval(crast, col);
                        pixel[8] = pval(crast, col + 1);
                    }
                    else
                    {
                        pixel[4] = pval(brast, col - 1);
                        pixel[5] = -1;
                        pixel[6] = pval(crast, col - 1);
                        pixel[7] = pval(crast, col);
                        pixel[8] = -1;
                    }
                }
                else if( row != (nrows - 1) )
                {
                    if( col == 0 )
                    {
                        pixel[1] = -1;
                        pixel[2] = pval(arast, col);
                        pixel[3] = pval(arast, col + 1);
                        pixel[4] = -1;
                        pixel[5] = pval(brast, col + 1);
                        pixel[6] = -1;
                        pixel[7] = pval(crast, col);
                        pixel[8] = pval(crast, col + 1);
                    }
                    else if( col != (ncols - 1) )
                    {
                        pixel[1] = pval(arast, col - 1);
                        pixel[2] = pval(arast, col);
                        pixel[3] = pval(arast, col + 1);
                        pixel[4] = pval(brast, col - 1);
                        pixel[5] = pval(brast, col + 1);
                        pixel[6] = pval(crast, col - 1);
                        pixel[7] = pval(crast, col);
                        pixel[8] = pval(crast, col + 1);
                    }
                    else
                    {
                        pixel[1] = pval(arast, col - 1);
                        pixel[2] = pval(arast, col);
                        pixel[3] = -1;
                        pixel[4] = pval(brast, col - 1);
                        pixel[5] = -1;
                        pixel[6] = pval(crast, col - 1);
                        pixel[7] = pval(crast, col);
                        pixel[8] = -1;
                    }
                }
                else
                {
                    pixel[6] = -1;
                    pixel[7] = -1;
                    pixel[8] = -1;
                    if( col == 0 )
                    {
                        pixel[1] = -1;
                        pixel[2] = pval(arast, col);
                        pixel[3] = pval(arast, col + 1);
                        pixel[4] = -1;
                        pixel[5] = pval(brast, col + 1);
                    }
                    else if( col != (ncols - 1) )
                    {
                        pixel[1] = pval(arast, col - 1);
                        pixel[2] = pval(arast, col);
                        pixel[3] = pval(arast, col + 1);
                        pixel[4] = pval(brast, col - 1);
                        pixel[5] = pval(brast, col + 1);
                    }
                    else
                    {
                        pixel[1] = pval(arast, col - 1);
                        pixel[2] = pval(arast, col);
                        pixel[3] = -1;
                        pixel[4] = pval(brast, col - 1);
                        pixel[5] = -1;
                    }
                }

//                 pixel[1] = (row == 0 || col == 0)                 ? -1 : pval(arast, col - 1);
//                 pixel[2] = (row == 0)                             ? -1 : pval(arast, col);
//                 pixel[3] = (row == 0 || col == (ncols-1))         ? -1 : pval(arast, col + 1);
//                 pixel[4] = (col == 0)                             ? -1 : pval(brast, col - 1);
//                 pixel[5] = (col == (ncols-1))                     ? -1 : pval(brast, col + 1);
//                 pixel[6] = (row == (nrows-1) || col == 0)         ? -1 : pval(crast, col - 1);
//                 pixel[7] = (row == (nrows-1))                     ? -1 : pval(crast, col);
//                 pixel[8] = (row == (nrows-1) || col == (ncols-1)) ? -1 : pval(crast, col + 1);

                cold = warm = shadow = nulo = 0;
                for( i = 1; i < 9; i++ )
                {
                    switch( pixel[i] )
                    {
                        case IS_COLD_CLOUD: cold++;     break;
                        case IS_WARM_CLOUD: warm++;     break;
                        case IS_SHADOW    : shadow++;   break;
                        default:            nulo++;     break;
                    }
                }
                lim = (int)(cold + warm + shadow + nulo)/2;

                /* Entra pixel[0] = 0 */
                if( nulo < lim )
                {
                    if( shadow >= (cold + warm) ) pixel[0] = IS_SHADOW;
                    else pixel[0] = (warm > cold) ? IS_WARM_CLOUD : IS_COLD_CLOUD;
                }
            }
            if( pixel[0] != 0 )
            {
                ((CELL *) tmp.rast)[col] = pixel[0];
            }
            else
            {
                G_set_c_null_value((CELL *) tmp.rast + col, 1);
            }
        }
        if (G_put_raster_row(tmp.fd, tmp.rast, CELL_TYPE) < 0)
        {
            G_fatal_error(_("Cannot write to <%s>"), tmp.name);
        }
    }

    G_free(arast);
    G_free(brast);
    G_free(crast);
    G_close_cell(out->fd);

    G_free(tmp.rast);
    G_close_cell(tmp.fd);

    G_remove("cats", out->name);
    G_remove("cell", out->name);
    G_remove("cellhd", out->name);
    G_remove("cell_misc", out->name);
    G_remove("hist", out->name);

    G_rename("cats", tmp.name, out->name);
    G_rename("cell", tmp.name, out->name);
    G_rename("cellhd", tmp.name, out->name);
    G_rename("cell_misc", tmp.name, out->name);
    G_rename("hist", tmp.name, out->name);

    return;
}


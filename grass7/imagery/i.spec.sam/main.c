/* Spectral angle mapping
 * (c) 1998 Markus Neteler, Hannover, Germany
 *          neteler@geog.uni-hannover.de
 * 
 * Dependency to Meschach Lib removed in 2015
 *-------------------------------------------------------------------
 * V 0.1 - 26. Oct.1998
 *
 * * Based on Meschach Library (matrix operations)
 * * Copyright (C) 1993 David E. Steward & Zbigniew Leyk, all rights reserved.
 *
 * Cited references are from
 *   Steward, D.E, Leyk, Z. 1994: Meschach: Matrix computations in C.
 *      Proceedings of the centre for Mathematics and its Applicaions.
 *      The Australian National University. Vol. 32.
 *      ISBN 0 7315 1900 0
 *
 ********************************************************************/
             
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>

#include <grass/config.h>
#ifndef HAVE_LIBBLAS
#error GRASS is not configured with BLAS
#endif
#ifndef HAVE_LIBLAPACK
#error GRASS is not configured with LAPACK
#endif

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/imagery.h>
#include <grass/gmath.h>
#include <grass/glocale.h>
#include "local_proto.h"
#include "global.h"

struct GModule *module;

vec_struct *b, *Avector;
int matrixsize;

struct Ref Ref;

DCELL **cell;
int *cellfd;

DCELL **result_cell;
int *resultfd;

DCELL **error_cell;
int  error_fd;

char result_name[80];
char *result_prefix;

mat_struct  *open_files(char * matrixfile, char *img_grp);
float spectral_angle(vec_struct * Avector1, vec_struct * Avector2, int vtype);
DCELL myround(double x);

int main(int argc,char * argv[])
{
    int nrows, ncols;
    int row, col;
    int band;
    int i, j, error=0;
    /*char command[80]; rm by Yann temporarily see grayscale palette*/
    float anglefield[255][255];
    struct
    {
	struct Option *group, *matrixfile, *output;
    } parm;

    mat_struct *A; /*first use in open.c G_matrix_set()*/
    char *group;
    float spectangle; /*numerical value of the spectral angle*/

    G_gisinit (argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("spectral angle mapping"));
    module->description =
        _("Performs Spectral angle mapping on satellite/aerial images");


    parm.group = G_define_standard_option(G_OPT_I_GROUP);
    parm.group->description = "Imagery group to target for Spectral Mixture Analyis";

    parm.matrixfile = G_define_standard_option(G_OPT_F_INPUT);
    parm.matrixfile->description = "Matrix file containing spectral signatures";

    parm.output = G_define_option();
    parm.output->key = "result";
    parm.output->type = TYPE_STRING;
    parm.output->required = YES;
    parm.output->description = "Raster map prefix to hold spectral angles";

    if (G_parser(argc,argv))
	G_fatal_error("Parsing arguments error");

    result_prefix = parm.output->answer;

    G_message("%s",result_prefix);

    /*Creating A, the spectral signature matrix here*/
    A = open_files(parm.matrixfile->answer, parm.group->answer);
   /* Spectral Matrix is stored in A now */

  /* Check matrix orthogonality 
   * Ref: Youngsinn Sohn, Roger M. McCoy 1997: Mapping desert shrub rangeland
   *          using spectral unmixing and modeling spectral mixtrues with 
   *          TM data. Photogrammetric Engineering & Remote Sensing, Vol.63, No6.
   */
     
    G_message("/* Check matrix orthogonality*/"); 
    for (i = 0; i < Ref.nfiles; i++) /* Ref.nfiles = matrixsize*/
    {
     Avector = G_matvect_get_row(A, i);  /* go columnwise through matrix*/
     for (j = 0; j < Ref.nfiles ; j++)
	{
	 if (j !=i)
	    {
	     b = G_matvect_get_row(A, j);      /* compare with next col in A */
	     spectangle = spectral_angle(Avector, b, RVEC);
	     anglefield[i][j]= spectangle;
	     G_vector_free(b);
	    }
	}
     G_vector_free(Avector);
    }

    /* print out the result */
    G_message("Orthogonality check of Matrix A:\n");
    for (i = 0; i < Ref.nfiles ; i++)
      for (j = 0; j < Ref.nfiles ; j++)
	{
         if (j !=i)
         	G_message("  Angle between row %i and row %i: %g\n", (i+1), (j+1), anglefield[i][j]);
        }
    G_message("\n");
    
   /* check it */
   for (i = 0; i < Ref.nfiles ; i++)
     for (j = 0; j < Ref.nfiles ; j++)
       if (j !=i)
         if (anglefield[i][j] < 10.0)
         {
	     G_message("ERROR: Spectral entries row %i|%i in your matrix are linear dependent!\n",i,j);
	     error=1;
	 }

    if (!error)
	G_message("Spectral matrix is o.k. Proceeding...\n");
   
    /* check singular values of Matrix A
     * Ref: Boardman, J.W. 1989: Inversion of imaging spectrometry data
     *        using singular value decomposition.  IGARSS 1989: 12th Canadian
     *           symposium on Remote Sensing. Vol.4 pp.2069-2072
     */
    G_verbose_message("Singular values ouput vector init");
    double *svdvalues = G_alloc_vector(A->cols);
    G_verbose_message("Singular values of Matrix A: copy A into double **");
    double **Avals = G_alloc_matrix(A->cols,A->rows);
    int count=0;
    for (i = 0; i < A->cols ; i++){
     for (j = 0; j < A->rows ; j++){
      Avals[i][j] = A->vals[count];
      count++;
     }
    }
    G_verbose_message("Singular values of Matrix A: run svdval");
    if(G_math_svdval(svdvalues, Avals, A->cols, A->rows))
        G_fatal_error("Error in singular value decomposition, exiting...\n");
    G_verbose_message("/*display values (replace v_output() in original version)*/");
    /*display values (replace v_output() in original version)*/
    for(i=0;i<A->cols;i++)
        G_message("%f", svdvalues[i]);

    G_verbose_message("/* alright, start Spectral angle mapping */");
    /* alright, start Spectral angle mapping */
    nrows = Rast_window_rows();
    ncols = Rast_window_cols();
    
    G_verbose_message("Calculating for %i x %i = %i pixels:\n",nrows,ncols, (ncols * ncols));
    G_verbose_message("%s ... ", G_program_name());
    G_message("%s ... ", G_program_name());

    for (row = 0; row < nrows; row++)                 /* rows loop*/
    {
	G_percent(row, nrows, 2);
	for (band = 0; band < Ref.nfiles; band++)     /* get row for all bands*/
	    Rast_get_d_row (cellfd[band], cell[band], row);

	for (col = 0; col < ncols; col++)             /* cols loop, work pixelwise for all bands */
	{
	    /* get pixel values of each band and store in b vector: */
	     /*b = v_get(A->m);*/                   /* m=rows; dimension of vector = matrix size = Ref.nfiles*/
            b = G_vector_init(A->cols,A->cols,CVEC);
	    for (band = 0; band < Ref.nfiles-1; band++)
                 b->vals[band] = cell[band][col];  /* read input vector */
	   
            /* calculate spectral angle for current pixel
             * between pixel spectrum and reference spectrum
             * and write result in full degree */
             for (i = 0; i < Ref.nfiles; i++) /* Ref.nfiles = matrixsize*/
             {
              Avector = G_matvect_get_column(A, i);  /* go row-wise through matrix*/
              G_verbose_message("Av: %f %f %f %f",Avector->vals[0],Avector->vals[1],Avector->vals[2],Avector->vals[3]);
	      spectangle = spectral_angle(Avector, b, CVEC);
	      result_cell[i][col] = myround (spectangle);
	      G_vector_free(Avector);
             }
	     G_vector_free(b);
	     
	 } /* columns loop */

	/* write the resulting rows: */
        for (i = 0; i < Ref.nfiles; i++)
        for (i = 0; i < Ref.nfiles; i++)
          Rast_put_d_row (resultfd[i], result_cell[i]);

    } /* rows loop */
    G_percent(row, nrows, 2);
	
   /* close files */
    for (i = 0; i < Ref.nfiles; i++)
    {
	  Rast_close (resultfd[i]);
	  Rast_unopen(cellfd[i]);
	  /* make grey scale color table */
	  /*sprintf(result_name, "%s.%d", result_prefix, (i+1));	               
          sprintf(command, "r.colors map=%s color=grey >/dev/null", result_name);
          system(command);*/ /*Commented by Yann*/
          /* write a color table */
    }
    G_matrix_free(A);
    make_history(result_name, parm.group->answer, parm.matrixfile->answer);
    return(EXIT_SUCCESS);
} /* main*/


DCELL myround (double x)
  {
    DCELL n;
    
    if (x >= 0.0)
        n = x + .5;
    else
       {
        n = -x + .5;
        n = -n;
       }
    return n;
  }
                                        

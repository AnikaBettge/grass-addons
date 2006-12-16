/****************************************************************************
 *
 * MODULE:       r.eb.evapfr
 * AUTHOR(S):    Yann Chemin - ychemin@gmail.com
 * PURPOSE:      Calculates the evaporative fraction
 *               as seen in Bastiaanssen (1995) 
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>


double evap_fr(double r_net, double g0, double h0);
double soilmoisture( double evapfr );

int main(int argc, char *argv[])
{
	struct Cell_head cellhd; //region+header info
	char *mapset; // mapset name
	int nrows, ncols;
	int row,col;

	int verbose=1;
	int makin=0;//Makin Flag for root zone soil moisture output
	struct GModule *module;
	struct Option *input1, *input2, *input3, *output1, *output2;
	
	struct Flag *flag1, *flag2;	
	struct History history; //metadata
	
	/************************************/
	/* FMEO Declarations*****************/
	char *name;   // input raster name
	char *result1, *result2; //output raster name
	//File Descriptors
	int infd_rnet, infd_g0, infd_h0;
	int outfd1, outfd2;
	
	char *rnet,*g0,*h0;
	char *evapfr, *theta;	
	int i=0,j=0;
	
	void *inrast_rnet, *inrast_g0, *inrast_h0;
	unsigned char *outrast1, *outrast2;
	RASTER_MAP_TYPE data_type_rnet;
	RASTER_MAP_TYPE data_type_g0;
	RASTER_MAP_TYPE data_type_h0;
	/************************************/
	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("evaporative fraction, soil moisture, energy balance, SEBAL");
	module->description = _("evaporative fraction (Bastiaanssen, 1995) and root zone soil moisture (Makin, Molden and Bastiaanssen, 2001)");

	/* Define the different options */
	input1 = G_define_option() ;
	input1->key	   = _("rnet");
	input1->type       = TYPE_STRING;
	input1->required   = YES;
	input1->gisprompt  =_("old,cell,raster") ;
	input1->description=_("Name of the Net Radiation map [W/m2]");
	input1->answer     =_("rnet");

	input2 = G_define_option() ;
	input2->key        =_("g0");
	input2->type       = TYPE_STRING;
	input2->required   = YES;
	input2->gisprompt  =_("old,cell,raster");
	input2->description=_("Name of the soil heat flux map [W/m2]");
	input2->answer     =_("g0");

	input3 = G_define_option() ;
	input3->key        =_("h0");
	input3->type       = TYPE_STRING;
	input3->required   = YES;
	input3->gisprompt  =_("old,cell,raster");
	input3->description=_("Name of the sensible heat flux map [W/m2]");
	input3->answer     =_("h0");

	output1 = G_define_option() ;
	output1->key        =_("evapfr");
	output1->type       = TYPE_STRING;
	output1->required   = YES;
	output1->gisprompt  =_("new,cell,raster");
	output1->description=_("Name of the output evaporative fraction layer");
	output1->answer     =_("evapfr");

	output2 = G_define_option() ;
	output2->key        =_("theta");
	output2->type       = TYPE_STRING;
	output2->required   = NO;
	output2->gisprompt  =_("new,cell,raster");
	output2->description=_("Name of the output root zone soil moisture layer");
	output2->answer     =_("theta");
	
	
	flag1 = G_define_flag();
	flag1->key = 'm';
	flag1->description = _("root zone soil moisture output (Makin, Molden and Bastiaanssen, 2001)");
	
	flag2 = G_define_flag();
	flag2->key = 'q';
	flag2->description = _("Quiet");

	/********************/
	if (G_parser(argc, argv))
		exit (EXIT_FAILURE);

	rnet	 	= input1->answer;
	g0	 	= input2->answer;
	h0		= input3->answer;
	
	result1  = output1->answer;
	result2  = output2->answer;
	makin   = flag1->answer;
	verbose = (!flag2->answer);
	/***************************************************/
	mapset = G_find_cell2(rnet, "");
	if (mapset == NULL) {
		G_fatal_error(_("cell file [%s] not found"), rnet);
	}
	data_type_rnet = G_raster_map_type(rnet,mapset);
	if ( (infd_rnet = G_open_cell_old (rnet,mapset)) < 0)
		G_fatal_error (_("Cannot open cell file [%s]"), rnet);
	if (G_get_cellhd (rnet, mapset, &cellhd) < 0)
		G_fatal_error (_("Cannot read file header of [%s])"), rnet);
	inrast_rnet = G_allocate_raster_buf(data_type_rnet);
	/***************************************************/
	mapset = G_find_cell2 (g0, "");
	if (mapset == NULL) {
		G_fatal_error(_("cell file [%s] not found"),g0);
	}
	data_type_g0 = G_raster_map_type(g0,mapset);
	if ( (infd_g0 = G_open_cell_old (g0,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), g0);
	if (G_get_cellhd (g0, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), g0);
	inrast_g0 = G_allocate_raster_buf(data_type_g0);
	/***************************************************/
	mapset = G_find_cell2 (h0, "");
	if (mapset == NULL) {
		G_fatal_error(_("Cell file [%s] not found"), h0);
	}
	data_type_h0 = G_raster_map_type(h0,mapset);
	if ( (infd_h0 = G_open_cell_old (h0,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), h0);
	if (G_get_cellhd (h0, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), h0);
	inrast_h0 = G_allocate_raster_buf(data_type_h0);
	/***************************************************/
	G_debug(3, "number of rows %d",cellhd.rows);
	nrows = G_window_rows();
	ncols = G_window_cols();
	outrast1 = G_allocate_raster_buf(data_type_h0);
	if(makin){
		outrast2 = G_allocate_raster_buf(data_type_h0);
	}
	/* Create New raster files */
	if ( (outfd1 = G_open_raster_new (result1,data_type_h0)) < 0)
		G_fatal_error(_("Could not open <%s>"),result1);
	if(makin){
		if ( (outfd2 = G_open_raster_new (result2,data_type_h0)) < 0)
			G_fatal_error(_("Could not open <%s>"),result2);
	}
	/* Process pixels */
	for (row = 0; row < nrows; row++)
	{
		DCELL d;
		DCELL d_rnet;
		DCELL d_g0;
		DCELL d_h0;
		if(verbose)
			G_percent(row,nrows,2);
//		printf("row = %i/%i\n",row,nrows);
		/* read soil input maps */	
		if(G_get_raster_row(infd_rnet,inrast_rnet,row,data_type_rnet)<0)
			G_fatal_error(_("Could not read from <%s>"),rnet);
		if(G_get_raster_row(infd_g0,inrast_g0,row,data_type_g0)<0)
			G_fatal_error(_("Could not read from <%s>"),g0);
		if(G_get_raster_row(infd_h0,inrast_h0,row,data_type_h0)<0)
			G_fatal_error(_("Could not read from <%s>"),h0);
		/*process the data */
		for (col=0; col < ncols; col++)
		{
		//	printf("col=%i/%i ",col,ncols);
			d_rnet = ((DCELL *) inrast_rnet)[col];
 		//	printf("rnet = %5.3f", d_rnet);
			d_g0 = ((DCELL *) inrast_g0)[col];
 		//	printf(" g0 = %5.3f", d_g0);
			d_h0 = ((DCELL *) inrast_h0)[col];
 		//	printf(" h0 = %5.3f", d_h0);
			if(G_is_d_null_value(&d_rnet)){
				((DCELL *) outrast1)[col] = -999.99;
				if(makin){
					((DCELL *) outrast2)[col] = -999.99;
				}
			}else if(G_is_d_null_value(&d_g0)){
				((DCELL *) outrast1)[col] = -999.99;
				if(makin){
					((DCELL *) outrast1)[col] = -999.99;
				}
			}else if(G_is_d_null_value(&d_h0)){
				((DCELL *) outrast1)[col] = -999.99;
				if(makin){
					((DCELL *) outrast1)[col] = -999.99;
				}
			}else {
				/************************************/
				/* calculate soil heat flux	    */
				d = evap_fr(d_rnet,d_g0,d_h0);
		//		printf(" || d=%5.3f",d);
				((DCELL *) outrast1)[col] = d;
		//		printf(" -> %5.3f\n",d);
				if(makin){
					d = soilmoisture(d);
					((DCELL *) outrast2)[col] = d;
				}
			}
		//	if(row==50){
		//		exit(EXIT_SUCCESS);
		//	}
		}
		if (G_put_raster_row (outfd1, outrast1, data_type_h0) < 0)
			G_fatal_error(_("Cannot write to output raster file"));
		if(makin){
			if (G_put_raster_row (outfd2, outrast2, data_type_h0) < 0)
				G_fatal_error(_("Cannot write to output raster file"));
		}
	}

	G_free (inrast_rnet);
	G_free (inrast_g0);
	G_free (inrast_h0);
	G_close_cell (infd_rnet);
	G_close_cell (infd_g0);
	G_close_cell (infd_h0);
	
	G_free (outrast1);
	G_free (outrast2);
	if(makin){
		G_close_cell (outfd1);
		G_close_cell (outfd2);
	}

	G_short_history(result1, "raster", &history);
	G_command_history(&history);
	G_write_history(result1,&history);

	if(makin){
		G_short_history(result2, "raster", &history);
		G_command_history(&history);
		G_write_history(result2,&history);
	}
	exit(EXIT_SUCCESS);
}


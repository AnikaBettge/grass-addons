/****************************************************************************
 *
 * MODULE:       r.evapo.potrad
 * AUTHOR(S):    Yann Chemin - ychemin@gmail.com
 * PURPOSE:      Creates a map of potential evapotranspiration following
 *               the condition that all net radiation becomes ET
 *               (thus it can be called a "radiative ET pot")
 *
 * COPYRIGHT:    (C) 2006 by the Asian Institute of Technology, Thailand
 * 		 (C) 2002 by the GRASS Development Team
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

double solar_day(double lat, double doy, double tsw );
double biomass( double apar, double solar_day, double evap_fr, double light_use_ef );

int main(int argc, char *argv[])
{
	struct Cell_head cellhd; //region+header info
	char *mapset; // mapset name
	int nrows, ncols;
	int row,col;

	int verbose=1;
	struct GModule *module;
	struct Option *input1, *input2, *input3, *input4, *input5, *input6;
	struct Option *output1;
	
	struct Flag *flag1;	
	struct History history; //metadata
	
	/************************************/
	/* FMEO Declarations*****************/
	char *name;   // input raster name
	char *result1,*result2; //output raster name
	//File Descriptors
	int infd_fpar, infd_lightuseff, infd_lat, infd_doy, infd_tsw, infd_wat_avail;
	int outfd1;
	
	char *fpar,*lightuseff,*lat,*doy,*tsw, *wat_avail;
	int i=0,j=0;
	
	void *inrast_fpar, *inrast_lightuseff, *inrast_lat, *inrast_doy, *inrast_tsw, *inrast_wat_avail;
	unsigned char *outrast1;
	RASTER_MAP_TYPE data_type_output=DCELL_TYPE;
	RASTER_MAP_TYPE data_type_fpar;
	RASTER_MAP_TYPE data_type_lightuseff;
	RASTER_MAP_TYPE data_type_lat;
	RASTER_MAP_TYPE data_type_doy;
	RASTER_MAP_TYPE data_type_tsw;
	RASTER_MAP_TYPE data_type_wat_avail;
	/************************************/
	G_gisinit(argv[0]);

	module = G_define_module();
	module->keywords = _("biomass, fpar, yield");
	module->description = _("biomass growth calculation, precursor of yield calculation when processing crop ");

	/* Define the different options */
	input1 = G_define_option() ;
	input1->key	   = _("fpar");
	input1->type       = TYPE_STRING;
	input1->required   = YES;
	input1->gisprompt  =_("old,cell,raster") ;
	input1->description=_("Name of the fPAR map");
	input1->answer     =_("fpar");

	input2 = G_define_option() ;
	input2->key        =_("lightuseff");
	input2->type       = TYPE_STRING;
	input2->required   = YES;
	input2->gisprompt  =_("old,cell,raster");
	input2->description=_("Name of the light use efficiency map (UZB:cotton=1.9)");
	input2->answer     =_("lightuseff");

	input3 = G_define_option() ;
	input3->key        =_("lat");
	input3->type       = TYPE_STRING;
	input3->required   = YES;
	input3->gisprompt  =_("old,cell,raster");
	input3->description=_("Name of the degree latitude map [dd.ddd]");
	input3->answer     =_("lat");

	input4 = G_define_option() ;
	input4->key        =_("doy");
	input4->type       = TYPE_STRING;
	input4->required   = YES;
	input4->gisprompt  =_("old,cell,raster");
	input4->description=_("Name of the Day of Year map [0.0-366.0]");
	input4->answer     =_("doy");

	input5 = G_define_option() ;
	input5->key        =_("tsw");
	input5->type       = TYPE_STRING;
	input5->required   = YES;
	input5->gisprompt  =_("old,cell,raster");
	input5->description=_("Name of the single-way transmissivity map [0.0-1.0]");
	input5->answer     =_("tsw");

	input6 = G_define_option() ;
	input6->key        =_("wat_avail");
	input6->type       = TYPE_STRING;
	input6->required   = YES;
	input6->gisprompt  =_("old,cell,raster");
	input6->description=_("Value of the water availability map (1-water stress, can use evaporative fraction directly) [0.0;1.0]");
	input6->answer     =_("wat_avail");


	output1 = G_define_option() ;
	output1->key        =_("output");
	output1->type       = TYPE_STRING;
	output1->required   = YES;
	output1->gisprompt  =_("new,cell,raster");
	output1->description=_("Name of the output daily biomass growth layer");
	output1->answer     =_("biomass");

	flag1 = G_define_flag();
	flag1->key = 'q';
	flag1->description = _("Quiet");
	/********************/
	if (G_parser(argc, argv))
		exit (EXIT_FAILURE);

	fpar	 	= input1->answer;
	lightuseff 	= input2->answer;
	lat		= input3->answer;
	doy	 	= input4->answer;
	tsw	 	= input5->answer;
	wat_avail 	= input6->answer;
	
	result1  = output1->answer;
	verbose = (!flag1->answer);
	/***************************************************/
	mapset = G_find_cell2(fpar, "");
	if (mapset == NULL) {
		G_fatal_error(_("cell file [%s] not found"), fpar);
	}
	data_type_fpar = G_raster_map_type(fpar,mapset);
	if ( (infd_fpar = G_open_cell_old (fpar,mapset)) < 0)
		G_fatal_error (_("Cannot open cell file [%s]"), fpar);
	if (G_get_cellhd (fpar, mapset, &cellhd) < 0)
		G_fatal_error (_("Cannot read file header of [%s])"), fpar);
	inrast_fpar = G_allocate_raster_buf(data_type_fpar);
	/***************************************************/
	mapset = G_find_cell2 (lightuseff, "");
	if (mapset == NULL) {
		G_fatal_error(_("cell file [%s] not found"), lightuseff);
	}
	data_type_lightuseff = G_raster_map_type(lightuseff,mapset);
	if ( (infd_lightuseff = G_open_cell_old (lightuseff,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), lightuseff);
	if (G_get_cellhd (lightuseff, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), lightuseff);
	inrast_lightuseff = G_allocate_raster_buf(data_type_lightuseff);
	/***************************************************/
	mapset = G_find_cell2 (lat, "");
	if (mapset == NULL) {
		G_fatal_error(_("Cell file [%s] not found"), lat);
	}
	data_type_lat = G_raster_map_type(lat,mapset);
	if ( (infd_lat = G_open_cell_old (lat,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), lat);
	if (G_get_cellhd (lat, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), lat);
	inrast_lat = G_allocate_raster_buf(data_type_lat);
	/***************************************************/
	mapset = G_find_cell2 (doy, "");
	if (mapset == NULL) {
		G_fatal_error(_("Cell file [%s] not found"), doy);
	}
	data_type_doy = G_raster_map_type(doy,mapset);
	if ( (infd_doy = G_open_cell_old (doy,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), doy);
	if (G_get_cellhd (doy, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), doy);
	inrast_doy = G_allocate_raster_buf(data_type_doy);
	/***************************************************/
	mapset = G_find_cell2 (tsw, "");
	if (mapset == NULL) {
		G_fatal_error(_("Cell file [%s] not found"), tsw);
	}
	data_type_tsw = G_raster_map_type(tsw,mapset);
	if ( (infd_tsw = G_open_cell_old (tsw,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), tsw);
	if (G_get_cellhd (tsw, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), tsw);
	inrast_tsw = G_allocate_raster_buf(data_type_tsw);
	/***************************************************/
	mapset = G_find_cell2 (wat_avail, "");
	if (mapset == NULL) {
		G_fatal_error(_("Cell file [%s] not found"), wat_avail);
	}
	data_type_wat_avail = G_raster_map_type(wat_avail,mapset);
	if ( (infd_wat_avail = G_open_cell_old (wat_avail,mapset)) < 0)
		G_fatal_error(_("Cannot open cell file [%s]"), wat_avail);
	if (G_get_cellhd (wat_avail, mapset, &cellhd) < 0)
		G_fatal_error(_("Cannot read file header of [%s]"), wat_avail);
	inrast_wat_avail = G_allocate_raster_buf(data_type_wat_avail);
	/***************************************************/
	G_debug(3, "number of rows %d",cellhd.rows);
	nrows = G_window_rows();
	ncols = G_window_cols();
	outrast1 = G_allocate_raster_buf(data_type_output);
	/* Create New raster files */
	if ( (outfd1 = G_open_raster_new (result1,data_type_output)) < 0)
		G_fatal_error(_("Could not open <%s>"),result1);
	/* Process pixels */
	for (row = 0; row < nrows; row++)
	{
		DCELL d;
		DCELL d_fpar;
		DCELL d_lightuseff;
		DCELL d_lat;
		DCELL d_doy;
		DCELL d_tsw;
		DCELL d_solar;
		DCELL d_wat_avail;
		if(verbose)
			G_percent(row,nrows,2);
		/* read input maps */	
		if(G_get_raster_row(infd_fpar,inrast_fpar,row,data_type_fpar)<0)
			G_fatal_error(_("Could not read from <%s>"),fpar);
		if(G_get_raster_row(infd_lightuseff,inrast_lightuseff,row,data_type_lightuseff)<0)
			G_fatal_error(_("Could not read from <%s>"),lightuseff);
		if(G_get_raster_row(infd_lat,inrast_lat,row,data_type_lat)<0)
			G_fatal_error(_("Could not read from <%s>"),lat);
		if(G_get_raster_row(infd_doy,inrast_doy,row,data_type_doy)<0)
			G_fatal_error(_("Could not read from <%s>"),doy);
		if(G_get_raster_row(infd_tsw,inrast_tsw,row,data_type_tsw)<0)
			G_fatal_error(_("Could not read from <%s>"),tsw);
		if(G_get_raster_row(infd_wat_avail,inrast_wat_avail,row,data_type_wat_avail)<0)
			G_fatal_error(_("Could not read from <%s>"),wat_avail);
		/*process the data */
		for (col=0; col < ncols; col++)
		{
			d_fpar = ((DCELL *) inrast_fpar)[col];
			d_lightuseff = ((DCELL *) inrast_lightuseff)[col];
			d_lat = ((DCELL *) inrast_lat)[col];
			d_doy = ((DCELL *) inrast_doy)[col];
			d_tsw = ((DCELL *) inrast_tsw)[col];
			d_wat_avail = ((DCELL *) inrast_wat_avail)[col];
			if(G_is_d_null_value(&d_fpar)){
				((DCELL *) outrast1)[col] = -999.99;
			}else if(G_is_d_null_value(&d_lightuseff)){
				((DCELL *) outrast1)[col] = -999.99;
			}else if(G_is_d_null_value(&d_lat)){
				((DCELL *) outrast1)[col] = -999.99;
			}else if(G_is_d_null_value(&d_doy)){
				((DCELL *) outrast1)[col] = -999.99;
			}else if(G_is_d_null_value(&d_tsw)){
				((DCELL *) outrast1)[col] = -999.99;
			}else if(G_is_d_null_value(&d_wat_avail)){
				((DCELL *) outrast1)[col] = -999.99;
			}else {
				d_solar = solar_day(d_lat, d_doy, d_tsw );
				d = biomass(d_fpar,d_solar,d_wat_avail,d_lightuseff);
				((DCELL *) outrast1)[col] = d;
			}
		}
		if (G_put_raster_row (outfd1, outrast1, data_type_output) < 0)
			G_fatal_error(_("Cannot write to output raster file"));
	}

	G_free (inrast_fpar);
	G_free (inrast_lightuseff);
	G_free (inrast_lat);
	G_free (inrast_doy);
	G_free (inrast_tsw);
	G_free (inrast_wat_avail);
	G_close_cell (infd_fpar);
	G_close_cell (infd_lightuseff);
	G_close_cell (infd_lat);
	G_close_cell (infd_doy);
	G_close_cell (infd_tsw);
	G_close_cell (infd_wat_avail);
	
	G_free (outrast1);
	G_close_cell (outfd1);

	G_short_history(result1, "raster", &history);
	G_command_history(&history);
	G_write_history(result1,&history);

	exit(EXIT_SUCCESS);
}


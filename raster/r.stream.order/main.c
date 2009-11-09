
/****************************************************************************
 *
 * MODULE:			 r.stream.order
 * AUTHOR(S):		Jarek Jasiewicz jarekj amu.edu.pl
 *							 
 * PURPOSE:			Calculate Strahler's and Horton's streams order maps, Hack's main streams map and Shreeve's stream magnitude
*								It use r.stream.extract or r.watershed output files: stream, direction and accumulation.
*
* COPYRIGHT:		 (C) 2002,2009 by the GRASS Development Team
*
*								 This program is free software under the GNU General Public
*								 License (>=v2). Read the file COPYING that comes with GRASS
*								 for details.
*
 *****************************************************************************/
#define MAIN
#include <grass/glocale.h>
#include "global.h"

/*
 * main function *
 */
int main(int argc, char *argv[])
{

    struct GModule *module;	/* GRASS module for parsing arguments */
    struct Option *in_dir_opt, *in_stm_opt, *in_acc_opt, *out_str_opt, *out_shr_opt, *out_hck_opt, *out_hrt_opt;	/* options */
    struct Flag *out_back;	/* flags */
    int stream_num;

    /* initialize GIS environment */
    G_gisinit(argv[0]);		/* reads grass env, stores program name to G_program_name() */

    /* initialize module */
    module = G_define_module();
    module->keywords =
	_("stream, order, magnitude, Horton, Strahler, Shreeve");
    module->description =
	_("Calculate Strahler's and Horton's stream order Hack's main streams and Shreeve's stream magnitude. It use r.watershed or r.stream.extract output files: stream, direction and optionally accumulation. Otput data can be eighter from r.watershed or r.stream.extract but not from both together");

    /*input option direction is reqired, acummulation is optional */

    in_stm_opt = G_define_option();	/* input stream mask file - optional */
    in_stm_opt->key = "stream";	/* required if strahler stream order is calculated for existing stream network */
    in_stm_opt->type = TYPE_STRING;
    in_stm_opt->required = YES;	/* for now; TO DO: is planned to be optional */
    in_stm_opt->gisprompt = "old,cell,raster";
    in_stm_opt->description =
	"Name of stream mask input map (r.watershed or r.stream.extract)";

    in_dir_opt = G_define_option();	/* input directon file */
    in_dir_opt->key = "dir";
    in_dir_opt->type = TYPE_STRING;
    in_dir_opt->required = YES;
    in_dir_opt->gisprompt = "old,cell,raster";
    in_dir_opt->description =
	"Name of direction input map (r.watershed or r.stream.extract)";

    in_acc_opt = G_define_option();	/* input stream mask file - optional */
    in_acc_opt->key = "accum";	/* required if strahler stream order is calculated for existing stream network */
    in_acc_opt->type = TYPE_STRING;
    in_acc_opt->required = NO;	/* for now; TO DO: is planned to be optional */
    in_acc_opt->answer = NULL;
    in_acc_opt->gisprompt = "old,cell,raster";
    in_acc_opt->description =
	"(Not recommended) Name of accumulation file (r.watershed or r.stream.extract)";

    /*      output option - at least one is reqired */

    out_str_opt = G_define_option();	/*Strahler stream order */
    out_str_opt->key = "strahler";
    out_str_opt->type = TYPE_STRING;
    out_str_opt->required = NO;
    out_str_opt->answer = NULL;
    out_str_opt->gisprompt = "new,cell,raster";
    out_str_opt->description = "Name of Strahler's stream order output map";
    out_str_opt->guisection = _("Output options");

    out_shr_opt = G_define_option();
    out_shr_opt->key = "shreve";
    out_shr_opt->type = TYPE_STRING;
    out_shr_opt->required = NO;
    out_shr_opt->answer = NULL;
    out_shr_opt->gisprompt = "new,cell,raster";
    out_shr_opt->description = "Name of Shreve's stream magnitude output map";
    out_shr_opt->guisection = _("Output options");

    out_hrt_opt = G_define_option();
    out_hrt_opt->key = "horton";
    out_hrt_opt->type = TYPE_STRING;
    out_hrt_opt->required = NO;
    out_hrt_opt->answer = NULL;
    out_hrt_opt->gisprompt = "new,cell,raster";
    out_hrt_opt->description =
	"Name of Horton's stream order output map";
    out_hrt_opt->guisection = _("Output options");

    out_hck_opt = G_define_option();
    out_hck_opt->key = "hack";
    out_hck_opt->type = TYPE_STRING;
    out_hck_opt->required = NO;
    out_hck_opt->answer = NULL;
    out_hck_opt->gisprompt = "new,cell,raster";
    out_hck_opt->description =
	"Name of Hack's main streams output map";
    out_hck_opt->guisection = _("Output options");

    /* Define flags */
    out_back = G_define_flag();
    out_back->key = 'z';
    out_back->description = _("Create zero-value background instead of NULL");

    if (G_parser(argc, argv))	/* parser */
	exit(EXIT_FAILURE);

    G_get_window(&window);

    if (!out_str_opt->answer && !out_shr_opt->answer && !out_hck_opt->answer
	&& !out_hrt_opt->answer)
	G_fatal_error(_("You must select one or more output maps: strahler, horton, shreeve or hack"));
    

    /* stores input options to variables */
    in_dirs = in_dir_opt->answer;
    in_streams = in_stm_opt->answer;
    in_accum = in_acc_opt->answer;

    /* stores output options to variables */
    out_strahler = out_str_opt->answer;
    out_shreeve = out_shr_opt->answer;
    out_hack = out_hck_opt->answer;
    out_horton = out_hrt_opt->answer;
    out_zero = (out_back->answer != 0);

		if (out_strahler) {
 	if (G_legal_filename(out_strahler) < 0)
		G_fatal_error(_("<%s> is an illegal file name"), out_strahler);
	  }
		if (out_shreeve) {
	if (G_legal_filename(out_shreeve) < 0)
	  G_fatal_error(_("<%s> is an illegal file name"), out_shreeve);
		}
		if (out_hack) {
	if (G_legal_filename(out_hack) < 0)
		G_fatal_error(_("<%s> is an illegal file name"), out_hack);
		}
		if (out_horton) {
	if (G_legal_filename(out_horton) < 0)
		G_fatal_error(_("<%s> is an illegal file name"), out_horton);
		}
		
    nrows = G_window_rows();
    ncols = G_window_cols();

    create_base_maps();
	stream_num = stream_number();

	stack_max = stream_num;	/* stack's size depends on number of streams */
    init_streams(stream_num);
    find_nodes(stream_num);
    if ((out_hack || out_horton) && !in_accum)
   do_cum_length();
    if (out_strahler || out_horton)
	strahler();
    if (out_shreeve)
	shreeve();
    if (out_horton)
	horton();
    if (out_hack)
	hack();
    write_maps();

    exit(EXIT_SUCCESS);
}

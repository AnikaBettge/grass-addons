/*!
  \file map_obj.c
 
  \brief Define creation and interface functions for map objects.
  
  Map objects are considered to be surfaces, vector plots,
  or site files.

  COPYRIGHT: (C) 2008 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.

  Based on visualization/nviz/src/map_obj.c

  \author Updated/modified by Martin Landa <landa.martin gmail.com>

  \date 2008
*/

#include <stdlib.h>
#include <time.h>

#include <grass/gsurf.h>
#include <grass/gstypes.h>
#include <grass/glocale.h>

#include "local_proto.h"

/*!
  \brief Create a new map object which can be one of surf, vect, vol or site.

  This routine creates the object
  internally in the gsf library and links a new tcl/tk
  command to the general object command parser below.
  Optionally, a logical name may be specified for the new map
  object.  If no name is specified, a logical name is assigned to
  the new object automatically.  Note that maintaining unique
  logical names is not the responsibility of the library (currently).
  
  Initially map objects contain no data, use the
  attribute commands to set attributes such as topology,
  color, etc.

  \param data
  \param interp
  \param argc
  \param argv

  \return map object id
  \return -1 on error
*/
int new_map_obj(int type, const char *name,
		nv_data *data)
{
    int new_id;

    nv_clientdata *client_data;

/*
 * For each type of map obj do the following --
 *   1) Verify we havn't maxed out the number of
 *      allowed objects.
 *   2) Call the internal library to generate a new
 *      map object of the specified type.
 *   3) Create a new tcl/tk command with the new object
 *      id number and link it to the Nmap_obj_cmd routine
 *      below.
 */
    if (type == MAP_OBJ_SURF) {
	if (GS_num_surfs() >= MAX_SURFS) {
	    G_warning (_("Maximum surfaces loaded!"));
	    return -1;
	}

	new_id = GS_new_surface();

	if (new_id < 0) {
	    return -1;
	}
	
	if (name) {
	    /* map */
	    if (!set_attr(new_id, MAP_OBJ_SURF, ATT_TOPO, MAP_ATT, name,
			  data)) {
		return -1;
	    }
	}
	else {
	    /* constant */
	    if (!set_attr(new_id, MAP_OBJ_SURF, ATT_TOPO, CONST_ATT, "0",
			  data)) {
		return -1;
	    }
	}	  
    }

    /* Initialize the client data filled for the new map object */
    client_data = (nv_clientdata *) G_malloc (sizeof(nv_clientdata));

    if (name) {
	client_data->logical_name = G_store(name);
    }
    else {
	char temp_space[80];
	time_t tp;
	
	/* Need to generate a random id */
	time(&tp);
	switch (type) {
	case MAP_OBJ_SURF: {
	    sprintf(temp_space, "%s*%ld", "surface", tp);
	    break;
	}
	default: {
	    sprintf(temp_space, "%s*%ld", "unknown", tp);
	    break;
	}
	}
	client_data->logical_name = G_store (temp_space);
    }
    
    G_debug(3, "new_map_obj(): logical name=%s",
	    client_data->logical_name);
    
    GS_Set_ClientData(new_id, (void *) client_data);
    
    return new_id;
}

/*!
  Set map object attribute

  \param id map object id
  \param type map object type (MAP_OBJ_SURF, MAP_OBJ_VECT, ...)
  \param desc attribute descriptors
  \param src attribute sources
  \param str_value attribute value as string

  \return 1 on success
  \return 0 on failure
*/
int set_attr(int id, int type, int desc, int src, const char *str_value,
	     nv_data *data)
{
    int ret;
    float value;
    
    switch (type) {
    case(MAP_OBJ_SURF): {
	/* Basically two cases, either we are setting to a constant field, or
	 * we are loading an actual file. Setting a constant is the easy part
	 * so we try and do that first.
	 */
	if (src == CONST_ATT) {
	    /* Get the value for the constant
	     * Note that we require the constant to be an integer
	     */
	    value = (float) atof(str_value);
	    
	    /* Only special case is setting constant color.
	     * In this case we have to decode the constant Tcl
	     * returns so that the gsf library understands it.
	     */
	    if (desc == ATT_COLOR) {
		/* TODO check this - sometimes gets reversed when save state
		   saves a surface with constant color */
		/* Tcl code and C code reverse RGB to BGR (sigh) */
		int r, g, b;
		r = (((int) value) & 0xff0000) >> 16;
		g = (((int) value) & 0x00ff00) >> 8;
		b = (((int) value) & 0x0000ff);
		value = r + (g << 8) + (b << 16);
	    }
	    
	    /* Once the value is parsed, set it */
	    ret = GS_set_att_const(id, desc, value);
	}
	else if (src == MAP_ATT) {
	    ret = GS_load_att_map(id, str_value, desc);
	}
	
	/* After we've loaded a constant map or a file,
	 * may need to adjust resolution if we are resetting
	 * topology (for example)
	 */
	if (0 <= ret) {
	    if (desc == ATT_TOPO) {
		int rows, cols, max;
		int max2;
		
		/* If topology attribute is being set then need to set
		 * resolution of incoming map to some sensible value so we
		 * don't wait all day for drawing.
		 */
		GS_get_dims(id, &rows, &cols);
		max = (rows > cols) ? rows : cols;
		max = max / 50;
		if (max < 1)
		    max = 1;
		max2 = max / 5;
		if (max2 < 1)
		    max2 = 1;
		/* reset max to finer for coarse surf drawing */
		max = max2 + max2/2;
		if (max < 1)
		    max = 1;
		
		GS_set_drawres(id, max2, max2, max, max);
		GS_set_drawmode(id, DM_GOURAUD | DM_POLY | DM_GRID_SURF);
	    }
	    
	    /* Not sure about this next line, should probably just
	     * create separate routines to figure the Z range as well
	     * as the XYrange
	     */
	    update_ranges(data);
	    
	    break;
	}
	default: {
	    return 0;
	}
    }
    }
    
    return 1;
}

/*!
  \brief Set default map object attributes
*/
void set_att_default()
{
    float defs[MAX_ATTS];
    
    defs[ATT_TOPO] = 0;
    defs[ATT_COLOR] = DEFAULT_SURF_COLOR;
    defs[ATT_MASK] = 0;
    defs[ATT_TRANSP] = 0;
    defs[ATT_SHINE] = 60;
    defs[ATT_EMIT] = 0;

    GS_set_att_defaults(defs, defs);

    return;
}

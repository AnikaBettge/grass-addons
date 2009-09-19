#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>


					/* define */

/*define directions for code simplicity

directions according to r.watershed: MUST check all directions
|3|2|1|
|4| |8|
|5|6|7|

*/
#define SQRT2 1.414214
#define POINT struct points	
POINT {
	int r, c;
float cur_dist;
float target_elev;
float northing;
float easting;
	};
	
#define OUTLET struct outs
OUTLET { 
	int r, c;
	float northing;
	float easting;
	};	

					/* functions.c */ 

/* io.c */
int open_raster(char *mapname);
int create_maps(void);
void free_streams (void);
int write_distance(void);
int write_elevation(void);
int set_null_elev(void);

/* distance */
int find_outlets(void);
int reset_distance(void);
int fill_maps(OUTLET outlet);
int fifo_insert (POINT point);
POINT fifo_return_del (void);


				/* variables */

#ifdef MAIN
#	define GLOBAL
#else
#	define GLOBAL extern
#endif

GLOBAL char *in_dirs, *in_streams, *in_elev;	/* input dirrection and accumulation raster names*/
GLOBAL char *out_dist, *out_elev;
GLOBAL int zeros, outs, subs; /* flags */


GLOBAL CELL **dirs, **streams; /* matrix with input data*/
GLOBAL FCELL **elevation, **distance; 
/* streams and elevation are used to store internal data during processing */

GLOBAL int nrows, ncols; 

POINT *fifo_outlet;
int tail, head;
int outlets_num;
int fifo_max;
	
GLOBAL int out; /* number of strahler and horton outlets: index */
OUTLET *outlets;

GLOBAL struct History history;	/* holds meta-data (title, comments,..) */
GLOBAL struct Cell_head window;





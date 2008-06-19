#ifndef GRASS_NVIZ_H
#define GRASS_NVIZ_H

#include <grass/gsurf.h>
#include <grass/gstypes.h>

/*** Windows headers ***/
#if defined(OPENGL_WINDOWS)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  include <winnt.h>

/*** X Window System headers ***/
#elif defined(OPENGL_X11)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>         /* for XA_RGB_DEFAULT_MAP atom */
#  if defined(__vms)
#    include <X11/StdCmap.h>     /* for XmuLookupStandardColormap */
#  else
#    include <X11/Xmu/StdCmap.h> /* for XmuLookupStandardColormap */
#  endif
#  include <GL/glx.h>

/*** Mac headers ***/
#elif defined(OPENGL_AQUA)
#  define Cursor QDCursor
#  include <AGL/agl.h>
#  undef Cursor
#  include <ApplicationServices/ApplicationServices.h>

#else /* make sure only one platform defined */
#  error Unsupported platform, or confused platform defines...
#endif

#define MAP_OBJ_UNDEFINED 0
#define MAP_OBJ_SURF 1
#define MAP_OBJ_VOL 2
#define MAP_OBJ_VECT 3

#define RANGE (5 * GS_UNIT_SIZE)
#define RANGE_OFFSET (2 * GS_UNIT_SIZE)
#define ZRANGE (3 * GS_UNIT_SIZE)
#define ZRANGE_OFFSET (1 * GS_UNIT_SIZE)

#define DEFAULT_SURF_COLOR 0x33BBFF

#define RED_MASK 0x000000FF
#define GRN_MASK 0x0000FF00
#define BLU_MASK 0x00FF0000

#define FORMAT_PPM 1
#define FORMAT_TIF 2

/* data structures */
typedef struct{
    int id;
    float brt;
    float r, g, b;
    float ar, ag, ab;  /* ambient rgb */
    float x, y, z, w; /* position */
} light_data;

typedef struct {
    /* ranges */
    float zrange, xyrange;
    
    /* cplanes */
    int num_cplanes;
    int cur_cplane, cp_on[MAX_CPLANES];
    float cp_trans[MAX_CPLANES][3];
    float cp_rot[MAX_CPLANES][3];
    
    /* light */
    light_data light[MAX_LIGHTS];
    
    /* background color */
    int bgcolor;
} nv_data;

/* The following structure is used to associate client data with surfaces.
 * We do this so that we don't have to rely on the surface ID (which is libal to change
 * between subsequent executions of nviz) when saving set-up info to files.
 */

typedef struct {
    /* We use logical names to assign textual names to map objects.
       When Nviz needs to refer to a map object it uses the logical name
       rather than the map ID.  By setting appropriate logical names, we
       can reuse names inbetween executions of Nviz.  The Nviz library
       also provides a mechanism for aliasing between logical names.
       Thus several logical names may refer to the same map object.
       Aliases are meant to support the case in which two logical names
       happen to be the same.  The Nviz library automatically assigns
       logical names uniquely if they are not specified in the creation
       of a map object.  When loading a saved file containing several map
       objects, it is expected that the map 0bjects will be aliased to
       their previous names.  This ensures that old scripts will work.
    */
    
    char *logical_name;
    
} nv_clientdata;

struct render_window {
    Display *displayId;   /* display connection */
    GLXContext contextId; /* GLX rendering context */
    Pixmap pixmap;
    GLXPixmap windowId;
};

/* render.c */
struct render_window* Nviz_new_render_window();
void Nviz_init_render_window(struct render_window*);
void Nviz_destroy_render_window(struct render_window *);
int Nviz_create_render_window(struct render_window *, void *,
			      int, int);

int Nviz_make_current_render_window(const struct render_window *);

#endif /* GRASS_NVIZ_H */

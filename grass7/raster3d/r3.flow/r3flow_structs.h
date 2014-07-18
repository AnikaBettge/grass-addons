#ifndef R3FLOW_STRUCTS_H
#define R3FLOW_STRUCTS_H

#include <grass/raster3d.h>

struct Seed
{
    double x;
    double y;
    double z;
    int flowline;
    int flowaccum;
};

struct Integration
{
    int direction;
    char *unit;
    double step;
    double cell_size;
    int limit;
};

struct Array
{
    double *array;
    int sx;
    int sy;
    int sz;
};

struct Gradient_info
{
    int compute_gradient;
    RASTER3D_Map *velocity_maps[3];
    RASTER3D_Map *scalar_map;
    double neighbors_values[24];
    int neighbors_pos[3];
    int initialized;
};

#define ACCESS(arr, x, y, z) ((arr)->array[(arr)->sx * (arr)->sy * (z) + (arr)->sx * (y) + (x)])

#endif // R3FLOW_STRUCTS_H

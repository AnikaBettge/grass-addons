#!/usr/bin/env python
# -*- coding: utf-8 -*-

##############################################################################
#
# MODULE:       r.rand.weight
# AUTHOR(S):    paulo van Breugel <paulo at ecodiv.org>
# PURPOSE:      Create a layer with weighted random sample
# COPYRIGHT: (C) 2014 Paulo van Breugel
#            http://ecodiv.org
#            http://pvanb.wordpress.com/
#
#        This program is free software under the GNU General Public
#        License (>=v2). Read the file COPYING that comes with GRASS
#        for details.
##############################################################################

#%module
#% description: Weighted sample
#% keyword: raster
#% keyword: sample
#%end

#%option
#% key: weights
#% type: string
#% gisprompt: old,cell,raster
#% description: layer with weight
#% key_desc: raster
#% required: yes
#% multiple: no
#%end

#%option
#% key: output
#% type: string
#% gisprompt: new,cell,raster
#% description: output layer
#% key_desc: raster
#% required: yes
#% multiple: no
#%end

#%option
#% key: start
#% type: double
#% description: minimum weight
#% required: yes
#%end

#%option
#% key: end
#% type: double
#% description: maximum weight
#% required: yes
#%end

#%option
#% key: subsample
#% type: string
#% description: subsample
#% required: no
#% guisection: Sample options
#%end

#%option
#% key: seed
#% type: string
#% description: set seed for random number generation
#% answer: auto
#% required: no
#% guisection: Sample options
#%end

#%flag
#% key: n
#% description: set non-selected values to 0 (default to NULL)
#%end


# import libraries
import os
import sys
import uuid
import string
import atexit
import grass.script as grass

# Runs modules silently
os.environ['GRASS_VERBOSE']='-1'

clean_rast = set()
def cleanup():
    for rast in clean_rast:
        grass.run_command("g.remove",
        type="rast", name = rast, quiet = True)

# main function
def main():

    # check if GISBASE is set
    if "GISBASE" not in os.environ:
    # return an error advice
    grass.fatal(_("You must be in GRASS GIS to run this program"))

    # input raster map and parameters
    minval = options['start']
    maxval = options['end']
    weight = options['weights']
    outmap = options['output']
    subsample = options['subsample']
    seed = options['seed']
    flag_n = flags['n']

    # setup temporary files and seed
    tmp_map = "r_w_rand_" + str(uuid.uuid4())
    tmp_map = string.replace(tmp_map, '-', '_')

    # Compute minimum and maximum value raster
    minmax = grass.parse_command('r.univar',
        map = weight,
        flags='g')

    if seed == "auto":
        grass.mapcalc("$tmp_map = rand(float(${minval}),float(${maxval}))",
            seed='auto',
            minval = minval,
            maxval = maxval,
            tmp_map = tmp_map)
    else:
        grass.mapcalc("$tmp_map = rand(float(${minval}),float(${maxval}))",
            seed=int(seed),
            minval = minval,
            maxval = maxval,
            tmp_map = tmp_map)
    clean_rast.add(tmp_map)

    if flag_n:
        grass.mapcalc("${outmap} = if($tmp_map <= ${weight},1,0)",
            weight = weight,
            outmap = outmap,
            tmp_map = tmp_map)
    else:
        grass.mapcalc("${outmap} = if($tmp_map <= ${weight},1,null())",
            weight = weight,
            outmap = outmap,
            tmp_map = tmp_map)

    grass.run_command("g.remove", quiet=True, flags="f", type="raster", name=tmp_map)
    if not subsample == '':
        grass.run_command('r.null',
            map = outmap,
            setnull = 0)
        grass.run_command('r.random',
            input = outmap,
            n = subsample,
            raster_output = outmap,
            overwrite=True)
        if flag_n:
            grass.run_command('r.null',
                map = outmap,
                null = 0)

    grass.message("------------------")
    grass.message("Ready!")
    grass.message("The name of raster created is " + outmap)
    #if minval > minmax['min'] or maxval < minmax['max']:
    grass.warning("You defined the minimum and maximum weights as: "
        + minval + " & " + maxval + ". Value range of weight raster is: "
        + minmax['min'] + " - " + minmax['max'])

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())


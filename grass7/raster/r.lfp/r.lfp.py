#!/usr/bin/env python
############################################################################
#
# MODULE:       r.lfp
# AUTHOR(S):    Huidae Cho
# PURPOSE:      Calculates the longest flow path for a given outlet point.
#
# COPYRIGHT:    (C) 2014 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Calculates the longest flow path for a given outlet point.
#% keywords: hydrology
#% keywords: watershed
#%end
#%option G_OPT_R_INPUT
#% description: Name of input drainage direction map
#%end
#%option G_OPT_R_OUTPUT
#% description: Name for output longest flow path map
#%end
#%option G_OPT_M_COORDS
#% description: Coordinates of outlet point
#%end

import sys
import os
import grass.script as grass

def main():
    input = options["input"]
    output = options["output"]
    coords = options["coordinates"]

    calculate_lfp(input, output, coords)

def calculate_lfp(input, output, coords):
    prefix = "r_lfp_%d_" % os.getpid()

    outlet = prefix + "outlet"
    p = grass.feed_command("v.in.ascii", overwrite=True,
            input="-", output=outlet, separator=",")
    p.stdin.write(coords)
    p.stdin.close()
    p.wait()
    if p.returncode != 0:
        grass.fatal(_("Cannot create outlet vector map"))

    if grass.run_command("v.to.rast", overwrite=True,
            input=outlet, output=outlet, use="cat") != 0:
        grass.fatal(_("Cannot convert outlet vector to raster"))

    flds = prefix + "flds"
    if grass.run_command("r.stream.distance", overwrite=True, flags="om",
            stream_rast=outlet, direction=input, method="downstream",
            distance=flds) != 0:
        grass.fatal(_("Cannot create flow length downstream raster map"))

    flus = prefix + "flus"
    if grass.run_command("r.stream.distance", overwrite=True, flags="o",
            stream_rast=outlet, direction=input, method="upstream",
            distance=flus) != 0:
        grass.fatal(_("Cannot create flow length upstream raster map"))

    fldsus = prefix + "fldsus"
    if grass.run_command("r.mapcalc", overwrite=True,
            expression="%s=%s+%s" % (fldsus, flds, flus)) != 0:
        grass.fatal(_("Cannot create flds+flus raster map"))

    p = grass.pipe_command("r.info", flags="r", map=fldsus)
    max = ""
    for line in p.stdout:
        line = line.rstrip("\n")
        if line.startswith("max="):
            max = line.split("=")[1]
            break
    p.wait()
    if p.returncode != 0 or max == "":
        grass.fatal(_("Cannot find max flds+flus cell value"))

    max = float(max) + 1
    min = max - 2

    if grass.run_command("r.mapcalc",
            expression="%s=if(%s>=%f, 1, null())" % (output, fldsus, min)) != 0:
        grass.fatal(_("Cannot create longest flow path raster map"))

    grass.run_command("g.mremove", flags="f",
            type="rast,vect", pattern="%s*" % prefix)

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())

#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
############################################################################
#
# MODULE:       r.lake.series
# AUTHOR(S):    Vaclav Petras
# PURPOSE:      Fills lake at given point(s) to given levels.
#
# COPYRIGHT:    (C) 2013 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


#%module
#% description: Fills lake at given point(s) to given levels.
#% keywords: raster
#% keywords: hydrology
#% keywords: hazard
#% keywords: flood
#%end
#%option G_OPT_R_ELEV
#%end
#%option G_OPT_STRDS_OUTPUT
#% label: Name of the output space time raster dataset
#% description: The name of the dataset is used as a base name for created output maps. Map names will consist of a base name, underscore and water level value or number depending on -c flag.
#%end
#%option
#% key: start_water_level
#% type: double
#% label: Start water level
#% description: Units should be meters?
#% required: yes
#% guisection: Water
#%end
#%option
#% key: end_water_level
#% type: double
#% label: Final (maximal) water level
#% description: Units should be meters?
#% required: yes
#% guisection: Water
#%end
#%option
#% key: water_level_step
#% type: double
#% label: Water level step
#% description: Units should be meters?
#% guisection: Water
#% required: yes
#%end
#%option G_OPT_M_COORDS
#% label: Seed point coordinates
#% description: Either this coordinates pair or a seed map name have to be specified.
#% required: no
#% guisection: Water
#%end
#%option G_OPT_R_INPUT
#% key: seed_raster
#% label: Name of input raster map with given starting point(s) (at least 1 cell > 0)
#% description: Either this parameter or a coordinates pair have to be specified.
#% required: no
#% guisection: Water
#%end
#%option
#% key: time_step
#% type: double
#% label: Time increment
#% description: Time increment between two states (maps) used to register output maps in space-time raster dataset. Used together with time_units parameter.
#% required: no
#% answer: 30
#% options: 0-
#% guisection: Time
#%end
#%option
#% key: time_unit
#% type: string
#% label: Time units
#% description: Time units used to register output maps in space-time raster dataset. Used together with time_step parameter.
#% required: no
#% options: years,months,days,hours,minutes,seconds
#% answer: minutes
#% guisection: Time
#%end
#%option
#% key: nproc
#% type: integer
#% label: Number of processes to run in parallel (currently ignored)
#% required: no
#% answer: 1
#% options: 1-
#%end
#%flag
#% key: n
#% label: Use negative depth values for lake raster map
#% description: This flag is passed to r.lake module.
#%end
#%flag
#% key: c
#% label: Use map number instead of the water level in map name (currently ignored)
#% description: This names are always in the right alphabetical order and are also valid vector map names.
#%end

"""
Created on Tue Oct 15 21:18:00 2013

@author: Vaclav Petras <wenzeslaus gmail.com>
"""

# TODO: support parallel
# TODO: generate SQL valid map names (replace decimal dot by underscore)
# TODO: use numbers instead of water levels flag
# TODO: remove unused functions

import sys

from grass.script import core as gcore
import grass.temporal as tgis


def format_time(time):
    return '%05.2f' % time


def format_order(number, zeros):
    return str(number).zfill(zeros)


def frange(x, y, step):
    while x <= y:
        yield x
        x += step


def check_maps_exist(maps, mapset):
    for map_ in maps:
        if gcore.find_file(map_, element='cell', mapset=mapset)['file']:
            gcore.fatal(_("Raster map <%s> already exists. Change the base name or allow overwrite.") % map_)


def main():
    options, flags = gcore.parser()

    elevation = options['elevation']
    strds = options['output']
    basename = strds
    start_water_level = float(options['start_water_level'])
    end_water_level = float(options['end_water_level'])
    water_level_step = float(options['water_level_step'])
    #if options['coordinates']:
    #    options['coordinates'].split(',')
    # passing coordinates parameter as is
    coordinates = options['coordinates']
    seed_raster = options['seed_raster']
    if seed_raster and coordinates:
        gcore.fatal(_("Both seed raster and coordinates cannot be specified"
                      " together, please specify only one of them."))

    time_unit = options['time_unit']
    time_step = options['time_step']
    if time_step <= 0:
        gcore.fatal(_("Time step must be greater than zero."
                      " Please specify number > 0."))

    mapset = gcore.gisenv()['MAPSET']
    title = _("r.lake series")
    desctiption = _("r.lake series")

    water_levels = [step for step in frange(start_water_level,
                                            end_water_level, water_level_step)]
    outputs = ['%s%s%s' % (basename, '_', water_level)
               for water_level in water_levels]

    if not gcore.overwrite():
        check_maps_exist(outputs, mapset)

    kwargs = {}
    flags = ''
    if seed_raster:
        kwargs['seed'] = seed_raster
    elif coordinates:
        kwargs['xy'] = coordinates
    if 'n' in flags:
        flags += 'n'

    for i, water_level in enumerate(water_levels):
        print outputs[i]
        gcore.run_command('r.lake', elevation=elevation, lake=outputs[i],
                          wl=water_level,
                          overwrite=gcore.overwrite(),  # TODO: really works? Its seems that hardcoding here False does not prevent overwriting.
                          **kwargs)

    gcore.info(_("Registering created maps into temporal dataset..."))

    # Make sure the temporal database exists
    tgis.init()

    tgis.open_new_space_time_dataset(strds, type='strds',
                                     temporaltype='relative',
                                     title=title, descr=desctiption,
                                     semantic='sum', dbif=None,
                                     overwrite=gcore.overwrite())
    # we must start from 1 because there is a bug in register_maps_in_space_time_dataset
    tgis.register_maps_in_space_time_dataset(
        type='rast', name=basename, maps=','.join(outputs), start=1, end=None,
        unit=time_unit, increment=time_step, dbif=None, interval=False)
if __name__ == '__main__':
    sys.exit(main())

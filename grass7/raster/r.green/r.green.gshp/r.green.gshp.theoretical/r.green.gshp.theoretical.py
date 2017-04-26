#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
############################################################################
#
# MODULE:      r.green.geothermal.potential
# AUTHOR(S):   Pietro Zambelli
# PURPOSE:     Calculate the Near Surface Geothermal Energy potential
# COPYRIGHT:   (C) 2017 by the GRASS Development Team
#
#              This program is free software under the GNU General Public
#              License (>=v2). Read the file COPYING that comes with GRASS
#              for details.
#
#############################################################################
#

#%module
#% description: Calculate the Ground Source Heat Pump potential
#% keywords: raster
#%end

##
## REQUIRED INPUTS
##
#%option G_OPT_R_INPUT
#% key: ground_conductivity
#% description: Raster with depth-averaged ground thermal conductivity λ [W m-1 K-1]
#% required: yes
#%end

##
## OPTIONAL INPUTS
##
#%option
#% key: heating_season
#% type: double
#% key_desc: double
#% description: Heating Season [0-365] days
#% required: no
#% options: 0-365
#% answer: 180.
#% guisection: Demand
#%end

#%option
#% key: power_value
#% type: double
#% key_desc: double
#% description: Power value in kW
#% required: no
#% answer: nan
#% guisection: Demand
#%end

#%option G_OPT_R_INPUT
#% key: ground_capacity_rast
#% description: Raster with depth-averaged ground thermal capacity ρc  [MJ m-3 K-1]
#% required: no
#% guisection: Ground
#%end

#%option
#% key: ground_capacity_value
#% type: double
#% key_desc: double
#% description: Value with depth-averaged ground thermal capacity ρc  [MJ m-3 K-1]
#% required: no
#% answer: 2.5
#% guisection: Ground
#%end


#%option G_OPT_R_INPUT
#% key: ground_temp_rast
#% description: Raster with the initial ground temperature T0 [°C]
#% required: no
#% guisection: Ground
#%end

#%option
#% key: ground_temp_value
#% type: double
#% key_desc: double
#% description: Value with the initial ground temperature T0 [°C]
#% required: no
#% answer: 10.
#% guisection: Ground
#%end

#%option
#% key: borehole_radius
#% type: double
#% key_desc: double
#% description: Borehole radius [m]
#% required: no
#% answer: 0.075
#% guisection: Borehole
#%end

#%option
#% key: borehole_resistence
#% type: double
#% key_desc: double
#% description: Borehole thermal resistence [m K W-1]
#% required: no
#% answer: nan
#% guisection: Borehole
#%end

#%option
#% key: borehole_length
#% type: double
#% key_desc: double
#% description: Borehole length [m]
#% required: no
#% answer: 100
#% guisection: Borehole
#%end

#%option
#% key: pipe_radius
#% type: double
#% key_desc: double
#% description: Pipe radius [m]
#% required: no
#% answer: 0.016
#% guisection: BHE
#%end

#%option
#% key: number_pipes
#% type: integer
#% key_desc: integer
#% description: Number of pipes in the borehole
#% required: no
#% answer: 4
#% guisection: BHE
#%end

#%option
#% key: grout_conductivity
#% type: double
#% key_desc: double
#% description: Thermal conductivity of the borehole filling (geothermal grout) [W m-1 K-1]
#% required: no
#% answer: 2
#% guisection: BHE
#%end

#%option
#% key: fluid_limit_temperature
#% type: double
#% key_desc: double
#% description: Minimum or maximum fluid temperature [°C]
#% required: no
#% answer: -2
#% guisection: BHE
#%end

#%option
#% key: lifetime
#% type: integer
#% key_desc: integer
#% description:  Simulated lifetime of the plant [years]
#% required: no
#% answer: 50
#% guisection: BHE
#%end

#%option G_OPT_R_OUTPUT
#% key: power
#% type: string
#% key_desc: name
#% description: Name of output raster map with the geothermal power potential [W]
#% required: yes
#%end

#%option G_OPT_R_OUTPUT
#% key: energy
#% type: string
#% key_desc: name
#% description: Name of output raster map with the geothermal energy potential [MWh]
#% required: yes
#%end

#%option G_OPT_R_OUTPUT
#% key: length
#% type: string
#% key_desc: name
#% description: Name of output raster map with the geothermal length of the BHE [m]
#% required: no
#%end

#%flag
#% key: d
#% description: Debug with intermediate maps
#%end

from __future__ import print_function

import os
import sys
import atexit

# import grass libraries
from grass.script import core as gcore
from grass.pygrass.utils import set_path

try:
    # set python path to the shared r.green libraries
    set_path('r.green', 'libgshp', '..')
    set_path('r.green', 'libgreen', os.path.join('..', '..'))
    from libgreen.utils import cleanup
    from libgshp import gpot
except ImportError:
    try:
        set_path('r.green', 'libgshp', os.path.join('..', 'etc', 'r.green'))
        set_path('r.green', 'libgreen', os.path.join('..', 'etc', 'r.green'))
        from libgreen.utils import cleanup
        from libgshp import gpot
    except ImportError:
        gcore.warning('libgreen and libhydro not in the python path!')


def rast_or_numb(rast, numb, opts):
    """Return a float or a string with the raster name.
    """
    return opts[rast] if opts[rast] else float(opts[numb])


def main(opts, flgs):
    """
    Parameters
    ----------
    heating_season: int [days]
        Number of heating days, default: 180 days
    ground_conductivity: float [W m-1 K-1]
        Depth averaged thermal conductivity
    ground_capacity: float []
        Depth averaged thermal capacity
    lifetime: int [years]
        Simulated lifetime of the plant, default: 50 years
    borehole_radius: float [m]
        Borehole radius
    pipe_radius: float [m]
        Pipe radius, default:
    number_pipes: int
        Number of pipes in the borehole, default: 4
    grout_conductivity: [W m-1 K-1]
        Thermal conductivity of the borehole filling (geothermal grout).
        Default: 2
    borehole_resistence: [m K W-1]
        Borehole thermal resistence
    borehole_length: [m]
        Borehole length, default: 100m
    ground_temperature: [°C]
        Initial ground temperature, default: 10 °C
    fluid_limit_temperature: [°C]
        Minimum or maximum fluid temperature, default: -2 °C
    """
    pid = os.getpid()
    DEBUG = flags['d']
    OVER = gcore.overwrite()
    tmpbase = "tmprgreen_%i" % pid
    atexit.register(cleanup, pattern=(tmpbase + '*'), debug=DEBUG)

    heating_season = float(opts['heating_season']) * 24 * 60 * 60
    lifetime = float(opts['lifetime']) * 365 * 24 * 60 * 60

    # ================================================
    # GROUND
    # get raster or scalar value
    ground_conductivity = opts['ground_conductivity']
    ground_capacity = rast_or_numb('ground_capacity_rast',
                                   'ground_capacity_value', opts)
    ground_temperature = rast_or_numb('ground_temp_rast',
                                      'ground_temp_value', opts)

    # ================================================
    # BHE
    pipe_radius = float(opts['pipe_radius'])
    number_pipes = float(opts['number_pipes'])
    grout_conductivity = float(opts['grout_conductivity'])
    fluid_limit_temperature = float(opts['fluid_limit_temperature'])

    # ================================================s
    # BOREHOLE
    borehole_radius = float(opts['borehole_radius'])
    borehole_length = float(opts['borehole_length'])
    if opts['borehole_resistence'] == 'nan':
        borehole_resistence = gpot.get_borehole_resistence(borehole_radius,
                                                           pipe_radius,
                                                           number_pipes,
                                                           grout_conductivity)
    else:
        borehole_resistence = float(opts['borehole_resistence'])

    # START COMPUTATIONS
    uc = tmpbase + '_uc'
    gpot.r_norm_time(uc, heating_season, borehole_radius,
                     ground_conductivity, ground_capacity, execute=True,
                     overwrite=OVER)
    us = tmpbase + '_us'
    gpot.r_norm_time(us, lifetime, borehole_radius,
                     ground_conductivity, ground_capacity, execute=True,
                     overwrite=OVER)

    gmax = tmpbase + '_gmax'
    tc = heating_season / (365. * 24 * 60 * 60.)
    gpot.r_norm_thermal_alteration(gmax, tc, uc, us, execute=True,
                                   overwrite=OVER)

    power = opts['power']
    gpot.r_power(power, tc, ground_conductivity, ground_temperature,
                 fluid_limit_temperature, borehole_length, borehole_resistence,
                 gmax, execute=True, overwrite=OVER)

    energy = opts['energy']
    gpot.r_energy(energy, power, execute=True, overwrite=OVER)


if __name__ == "__main__":
    options, flags = gcore.parser()
    main(options, flags)
    sys.exit(0)

#!/usr/bin/env python

##############################################################################
#
# MODULE:      r.shalstab
# VERSION:     1.0
# AUTHOR(S):   Andrea Filipello & Daniele Strigaro
# PURPOSE:     A model based on the algorithm developet by Montgomery and Dietrich (1994)
# COPYRIGHT:   (C) 2013 by Andrea Filipello & Daniele Strigaro
#              andrea.filipello@gmail.com; daniele.strigaro@gmail.com
#
#              This program is free software under the GNU General Public
#              License (>=v2). Read the file COPYING that comes with GRASS
#              for details.
#
##############################################################################

#%module
#%  description: A model for shallow landslide susceptibility
#%  keywords: raster, critical rainfall, landslide
#%end
#%option
#%  key: dem
#%  type: string
#%  gisprompt: old,raster,raster
#%  key_desc: name
#%  description: Name of input elevation raster map
#%  required: yes
#%end
#%option
#%  key: phy
#%  type: string
#%  gisprompt: old,raster,raster
#%  key_desc: name
#%  description: Soil friction angle (angle)
#%  required: yes
#%end
#%option
#%  key: c_soil
#%  type: string
#%  gisprompt: old,raster,raster
#%  key_desc: name
#%  description: Soil cohesion (N/m^2)
#%  required: yes
#%end
#%option
#%  key: gamma
#%  type: string
#%  gisprompt: old,raster,raster
#%  key_desc: name
#%  description: Soil density(Kg/m^3)
#%  required: yes
#%end
#%option
#%  key: z
#%  type: string
#%  gisprompt: old,raster,raster
#%  key_desc: name
#%  description: Vertical thickness of soil (m)
#%  required: yes
#%end
#%option
#%  key: k
#%  type: string
#%  gisprompt: old,raster,raster
#%  key_desc: name
#%  description: hydraulic conductivity (m/h)
#%  required: yes
#%end
#%option
#%  key: gamma_wet
#%  type: string
#%  gisprompt: old,raster,raster
#%  key_desc: name
#%  description: Wet soil density (kg/m^3)
#%  answer: 2100
#%  required: no
#%end
#%option
#%  key: root
#%  type: string
#%  gisprompt: old,raster,raster
#%  key_desc: name
#%  description: Root cohesion k (N/m^2)
#%  required: no
#%end
##############################################################################
# output 
##############################################################################
#%option
#% key: susceptibility
#% type: string
#% gisprompt: new,cell,raster
#% key_desc: susceptibility
#% description: Name for output landslide susceptibility map (from 1 to 7)
#% required : yes
#%end
#%option
#% key: critic_rain
#% type: string
#% gisprompt: new,cell,raster
#% key_desc: critical rain
#% description: Name for output critical rainfall map (mm/day)
#% required : yes
#%END

import sys
import os

try:
    import grass.script as grass
except:
    try:
        from grass.script import core as grass
    except:
        sys.exit("grass.script can't be imported.")

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)


def main():
    r_elevation = options['dem'].split('@')[0]
    mapname = options['dem'].replace("@", " ")
    mapname = mapname.split()
    mapname[0] = mapname[0].replace(".", "_")
    c_soil = options['c_soil']
    phy = options['phy']
    gamma = options['gamma']
    gamma_wet = options['gamma_wet']
    z = options['z']
    #b = options['b']
    k = options['k']
    root = options['root']
    susceptibility = options['susceptibility']
    critic_rain = options['critic_rain']
    if root == '':
        root = 0
    if gamma_wet == '':
        gamma_wet = 2100
    # calculate slope
    grass.run_command('r.slope.aspect', elevation=r_elevation, slope='slopes',
                      min_slp_allowed=0.0, overwrite='True') #format = 'degrees', precision = float, 
    # calculate soil transmissivity T (m^2/day)
    grass.mapcalc("T=$k*24*$z*cos(slopes)", k=k, z=z)
    # calculate dimensionless
    grass.mapcalc("C=($root+$c_soil)/($z*cos(slopes)*9.81*$gamma_wet)",
                  root=root, c_soil=c_soil, z=z, gamma_wet=gamma_wet)
    # calculate contribution area

    grass.run_command('r.watershed', elevation=r_elevation,
                      accumulation='accum')

    grass.mapcalc("A=accum*((ewres()+nsres())/2)*((ewres()+nsres())/2)")
    # calculate 1(m/day)
    grass.mapcalc("i_crit_m=T*sin(slopes)*((ewres()+nsres())/2)/A*($gamma/1000*(1-(1-C/(sin(slopes)*cos(slopes)))*tan(slopes)/tan($phy)))",
                  phy=phy, gamma=gamma)
    # Calculation of critical rain (mm / hr) and riclassification
    grass.mapcalc("i_cri_mm=i_crit_m*24000")
    reclass_rules = "0 thru 50 = 0\n50 thru 100 = 3\n100 thru 200 = 4\n" \
                    "200 thru 400 = 5\n400 thru 999 = 6"
    grass.write_command('r.reclass', input='i_cri_mm', output='i_recl',
                        overwrite='True', rules='-', stdin=reclass_rules)
    grass.mapcalc("copia_reclass=i_recl+0")
    grass.run_command('r.null', map='copia_reclass', null=0)
    grass.mapcalc("Stable=if (i_cri_mm>1000, 7, 0)")
    grass.mapcalc("Unstable=if (i_cri_mm<0, 1, 0)")
    grass.mapcalc("Icritica=copia_reclass+Unstable+Stable")
    colors_rules = "1 255:85:255\n2 255:0:0\n3 255:170:0\n4 255:255:0\n" \
                   "5 85:255:0\n6 170:255:255\n7 255:255:255"
    grass.write_command('r.colors', map='Icritica', rules='-',
                        stdin=colors_rules, quiet=True)

    grass.run_command('r.neighbors', input='Icritica', method='average',
                      size=3, output='I_cri_average')
    # rename maps
    grass.run_command('g.rename', rast=("I_cri_average", susceptibility))
    grass.run_command('g.rename', rast=("i_cri_mm", critic_rain))
    # remove temporary map
    grass.run_command('g.remove', rast=("A", "copia_reclass", "i_crit_m",
                                        "i_recl", "accum", "slopes", "Stable",
                                        "T", "Unstable", "C"))

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())

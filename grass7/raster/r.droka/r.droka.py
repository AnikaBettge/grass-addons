#!/usr/bin/env python
#
############################################################################
#
# MODULE:	r.droka
# AUTHOR(S):	original idea by: HUNGR (1993)
#		implementation by: 
#               Andrea Filipello -filipello@provincia.verbania.it
#               Daniele Strigaro - daniele.strigaro@gmail.com
# PURPOSE:	Calculates run-out distance of a falling rock mass
# COPYRIGHT:	(C) 2009 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################
#%Module
#%  description: Calculates run-out distance of a falling rock mass
#%  keywords: rock mass, rockfall
#%End
#%option
#% key: dem
#% type: string
#% gisprompt: old,cell,raster
#% description: Digital Elevation Model
#% required: yes
#%end
#%option
#% key: start
#% type: string
#% gisprompt: old,vector,vector
#% description: Name of existing rock mass start point 
#% required : yes
#%end
#%option
#% key: ang
#% type: double
#% description: Shadow angle
#% required: yes
#%end
#%option
#% key: red
#% type: double
#% description: Reduction parameter
#% options : 0-1
#% required: yes
#%end
#%option
#% key: m
#% type: double
#% description: Rock block mass (Kg/m^3)
#% required: yes
#%end
#%option
#% key: rocks
#% type: string
#% gisprompt: new,cell,raster
#% description: Rocks number
#% required : yes
#%end
#%option
#% key: vmax
#% type: string
#% gisprompt: new,cell,raster
#% description: Max translational velocity (corrected)
#% required : yes
#%end
#%option
#% key: vmean
#% type: string
#% gisprompt: new,cell,raster
#% description: Mean translational velocity (corrected)
#% required : yes
#%end
#%option
#% key: emax
#% type: string
#% gisprompt: new,cell,raster
#% description: Max kinematic energy (kJ) (corrected)
#% required: yes
#%end
#%option
#% key: emean
#% type: string
#% gisprompt: new,cell,raster
#% description: Mean kinematic energy (kJ) (corrected)
#% required: yes
#%end
#option
# key: x
# type: double
# description: Est coordinate of source point
# required: no
#end
#option
# key: y
# type: double
# description: North coordinate of source point
# required: no
#end
#option
# key: z
# type: double
# description: Elevation of source point
# required: no
#end

import os, sys, time, math , string, re
from grass.script import array as garray
import numpy as np
try:
    import grass.script as grass
except:
    try:
        from grass.script import core as grass
	#from grass.script import core as gcore
    except:
        sys.exit("grass.script can't be imported.")

if not os.environ.has_key("GISBASE"):
    print "You must be in GRASS GIS to run this program."
    sys.exit(1)

def main():
    
    # leggo variabili
    r_elevation = options['dem'].split('@')[0]
    mapname = options['dem'].replace("@", " ")
    mapname = mapname.split()
    mapname[0] = mapname[0].replace(".", "_")
    start = options['start']
    start_ = start.split('@')[0]
    gfile = grass.find_file(start, element = 'vector')
    if not gfile['name']:
        grass.fatal(_("Vector map <%s> not found") % infile)

    #x = options['x']
    #y = options['y']
    #z = options['z']
    ang = options['ang']
    red = options['red']
    m = options['m']
    rocks = str(options['rocks'])
    vMax = str(options['vmax'])
    vMean = str(options['vmean'])
    eMax = str(options['emax'])
    eMean = str(options['emean'])

    #print 'x = ' , x
    #print 'y = ' , y
    #print 'z = ' , z
    print 'ang = ' , ang
    print 'red = ' , red
    print 'm = ' , m
    
    #creo raster (che sara' il DEM di input) con valore 1
    grass.mapcalc('uno=$dem*0+1', 
        dem = r_elevation)     
    what = grass.read_command('r.what', map=r_elevation, points=start)
    quota = what.split('\n')

    #array per la somma dei massi
    tot = garray.array()
    tot.read(r_elevation)
    tot[...] = (tot*0.0).astype(float)
    somma = garray.array()

    #array per le velocita
    velocity = garray.array()
    velMax = garray.array()
    velMean = garray.array()

    #array per energia
    energy = garray.array()
    enMax = garray.array()
    enMean = garray.array()

    for i in xrange(len(quota)-1):
        z = float(quota[i].split('||')[1])
        point = quota[i].split('||')[0]
        x = float(point.split('|')[0])
        y = float(point.split('|')[1])
        print x,y,z
        # Calcolo cost (sostituire i punti di partenza in start_raster al pusto di punto)
        grass.run_command('r.cost' , 
            flags="k",  
            input = 'uno',
            output = 'costo',
            start_coordinates = str(x)+','+str(y),
            overwrite = True ) 


        #trasforma i valori di distanza celle in valori metrici utilizzando la risoluzione raster
        grass.mapcalc('costo_m=costo*(ewres()+nsres())/2',
             overwrite = True)

        # calcola A=tangente angolo visuale (INPUT) * costo in metri   
        grass.mapcalc('A=tan($ang)*costo_m',
            ang = ang,
             overwrite = True)
        grass.mapcalc('C=$z-A',
            z = z,
            overwrite = True)
        grass.mapcalc('D=C-$dem',
            dem = r_elevation,
             overwrite = True)
        # area di espansione
        grass.mapcalc('E=if(D>0,1,null())',
             overwrite = True)
        # valore di deltaH (F)
        grass.mapcalc('F=D*E',
             overwrite = True)

        # calcolo velocita
        grass.mapcalc('vel = $red*sqrt(2*9.8*F)',
            red = red, overwrite = True)
        velocity.read('vel')
        velMax[...] = (np.where(velocity>velMax,velocity,velMax)).astype(float)
        velMean[...] = (velocity + velMean).astype(float)

        #calcolo numero massi
        grass.mapcalc('somma=if(vel>0,1,0)', overwrite = True)
        somma.read('somma') 
        tot[...] = (somma + tot).astype(float)

        # calcolo energia
        grass.mapcalc('en=$m*9.8*F/1000',
            m = m,
            overwrite = True)
        energy.read('en')
        enMax[...] = (np.where(energy>enMax,energy,enMax)).astype(float)
        enMean[...] = (energy + enMean).astype(float)

    tot.write(rocks)
    velMax.write(vMax)
    velMean[...] = (velMean/i).astype(float)
    velMean.write(vMean)
    enMax.write(eMax)
    enMean[...] = (enMean/i).astype(float)
    enMean.write(eMean)
    grass.run_command('g.remove' , 
        rast=(
            'uno',
            'costo',
            'costo_m',
            'A',
            'C',
            'D',
            'E',
            'F',
            'en',
            'vel',
            'somma'))

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())



# codice da aggiungere per leggere la pendenza dl layer vettoriale
#ZN = grass.read_command("v.db.select", flags="c", map="geochimcal", col="ZN")
#ZN=(ZN.split("\n"))
#ZN= ZN[0:(len(ZN)-1)]
#print ZN
#['40', '55', '65', '158', '44', '282', '62', '83', '84', '97', '61', '58', '40', '54', '75', '129', #'77', '87', '74', '47', '58', '73', '64', '46', '63']

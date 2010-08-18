#!/usr/bin/python

############################################################################
#
# MODULE:       r.landscape.evol.py
# AUTHOR(S):    iullah
# COPYRIGHT:    (C) 2009 GRASS Development Team/iullah
#
#  description: Create raster maps of net erosion/depostion, the modified terrain surface (DEM) after net erosion/deposition using the USPED equation, bedrock elevations after soil production, and soil depth maps. This module uses appropriate flow on different landforms by default; however, singular flow regimes can be chosen instead. THIS SCRIPT WILL PRODUCE MANY TEMPORARY MAPS AND REQUIRES A LOT OF FREE FILE SPACE! 

#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################/
#%Module
#% description: Create raster maps of net erosion/depostion, the modified terrain surface (DEM) after net erosion/deposition using the USPED equation, bedrock elevations after soil production, and soil depth maps. This module uses appropriate flow on different landforms by default; however, singular flow regimes can be chosen instead. THIS SCRIPT WILL PRODUCE MANY TEMPORARY MAPS AND REQUIRES A LOT OF FREE FILE SPACE! 
#%End
#%option
#% key: elev
#% type: string
#% gisprompt: old,cell,raster
#% description: Input elevation map (DEM)
#% required : yes
#% guisection: Input
#%end
#%option
#% key: initbdrk
#% type: string
#% gisprompt: old,cell,raster
#% description: Initial bedrock elevations map (for first iteration only)
#% answer: 
#% required : yes
#% guisection: Input
#%end
#%option
#% key: prefx
#% type: string
#% description: Prefix for all output maps
#% answer: levol_
#% required : yes
#% guisection: Input
#%end
#%option
#% key: outdem
#% type: string
#% description: Name stem for output elevation map(s) (preceded by prefix and followed by numerical suffix if more than one iteration)
#% answer: elevation
#% required: yes
#% guisection: Input
#%end
#%option
#% key: outsoil
#% type: string
#% description: Name stem for the output soil depth map(s) (preceded by prefix and followed by numerical suffix if more than one iteration)
#% answer: soildepth
#% required: yes
#% guisection: Input
#%end
#%option
#% key: outbdrk
#% type: string
#% description: Name stem for the output bedrock map(s) (required if the -b option is NOT checked; preceded by prefix and followed by numerical suffix if more than one iteration)
#% answer: bedrock
#% required: no
#% guisection: Input
#%end
#%Option
#% key: statsout
#% type: string
#% description: Name for the statsout text file (optional, if none provided, a default name will be used)
#% required: no
#% guisection: Input
#%end
#%flag
#% key: g
#% description: -g Don't put header on statsout text file and always append data, even if stats file already exists (useful if script is being run by an outside program)
#% guisection: Input
#%end
#%flag
#% key: l
#% description: -l Don't output yearly soil depth maps
#% guisection: Input
#%end
#%flag
#% key: k
#% description: -k Keep all temporary maps (do not clean up)
#% guisection: Input
#%end
#%flag
#% key: e
#% description: -e Keep initial soil depths map (for year 0)
#% guisection: Input
#%end
#%flag
#% key: b
#% description: -b Enable experimental bedrock weathering (soil production)
#% guisection: Input
#%end
#%flag
#% key: p
#% description: -p Keep all slope maps 
#% guisection: Input
#%end
#%flag
#% key: n
#% description: -n Don't output yearly net elevation change (netchange) maps
#% guisection: Input
#%end

#%option
#% key: R
#% type: string
#% description: Rainfall (R factor) constant (AVERAGE FOR WHOLE MAP AREA)
#% answer: 5.66
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: K
#% type: string
#% gisprompt: old,cell,raster
#% description: Soil erodability index (K factor) map or constant
#% answer: 0.42
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: sdensity
#% type: string
#% gisprompt: old,cell,raster
#% description: Soil density constant (for conversion from mass to volume)
#% answer: 1.2184
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: C
#% type: string
#% gisprompt: old,cell,raster
#% description: Landcover index (C factor) map or constant
#% answer: 0.001
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: rain
#% type: string
#% gisprompt: old,cell,raster
#% description: Anuall precip totals map or constant (in meters per year)
#% answer: 0.2
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: raindays
#% type: string
#% description: Number of days of rain per year (integer)
#% answer: 100
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: infilt
#% type: string
#% description: Proportion of rain that infiltrates into the ground rather than runs off (0.0 to 1.0)
#% answer: 0.1
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: Kt
#% type: string
#% description: Stream transport efficiency variable (0.001 for a normal substrate (ie. sediment) to 0.000001 for a very hard substrate like bedrock)
#% answer: 0.001
#% options: 0.001,0.0001,0.00001,0.00001,0.000001
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: loadexp
#% type: string
#% description: Stream transport type varaible (1.5 for mainly bedload transport, 2.5 for mainly suspended load transport)
#% answer: 1.5
#% options: 1.5,2.5
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: kappa
#% type: string
#% description: Hillslope diffusion (Kappa) rate map or constant (meters per kiloyear)
#% answer: 1
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: alpha
#% type: string
#% description: Critical slope threshold for mass movement of sediment (in degrees above horizontal)
#% answer: 40
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: cutoff1
#% type: string
#% description: Flow accumultion breakpoint value for shift from diffusion to overland flow (sq. m uplsope area)
#% answer: 900
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: cutoff2
#% type: string
#% description: Flow accumultion breakpoint value for shift from overland flow to rill/gully flow (sq. m uplsope area)
#% answer: 11250
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: cutoff3
#% type: string
#% description: Flow accumultion breakpoint value for shift from rill/gully flow to stream flow (sq. m uplsope area)
#% answer: 225000
#% required : yes
#% guisection: Variables
#%end
#%option
#% key: number
#% type: integer
#% description: Number of iterations (cycles) to run
#% answer: 1
#% required : yes
#% guisection: Variables
#%end


#these are commented out as we currently utilize the profile curvature method described by Heimsath et al...
# 	#%option
# 	#% key: Ba
# 	#% type: string
# 	#% description: Rate of average soil production (Ba)
# 	#% answer: 0.00008
# 	#% required : yes
# 	#% guisection: Variables
# 	#%end
# 	#%option
# 	#% key: Bb
# 	#% type: string
# 	#% description: Relationship between soil depth and production rate (Bb)
# 	#% answer: 0.1
# 	#% required : yes
# 	#% guisection: Variables
# 	#%end

#%flag
#% key: w
#% description: -w Calcuate for only sheetwash across entire map
#% guisection: Flow_type
#%end
#%flag
#% key: r
#% description: -r Calcuate for only channelized flow across entire map
#% guisection: Flow_type
#%end
#%flag
#% key: d
#% description: -d Calcuate for only diffusive flow (soil creep) across entire map
#% guisection: Flow_type
#%end
#%flag
#% key: f
#% description: -f Use r.terrflow instead of r.watershed to calculate flow accumulation ( GRASS 6.3.x users MUST use this flag!)
#% answer: 0
#% guisection: Flow_type
#%end


import sys
import os
import subprocess
import tempfile
grass_install_tree = os.getenv('GISBASE')
sys.path.append(grass_install_tree + os.sep + 'etc' + os.sep + 'python')
import grass.script as grass

# first define some useful custom methods

# m is a message (as a string) one wishes to have printed in the output window
def grass_message(m):
	subprocess.Popen('g.message message="%s"' % m, shell='bash').wait()
	return

# m is grass (or bash) command to execute (written as a string). script will wait for grass command to finish
def grass_com(m):
	subprocess.Popen('%s' % m, shell='bash').wait()
	return

# m is grass (or bash) command to execute (written as a string). script will not wait for grass command to finish
def grass_com_nowait(m):
	subprocess.Popen('%s' % m, shell='bash')
	return

# m is a grass/bash command that will generate some list of keyed info to stdout, n is the character that seperates the key from the data, o is a defined blank dictionary to write results to
def out2dict(m, n, o):
    p1 = subprocess.Popen('%s' % m, stdout=subprocess.PIPE, shell='bash')
    p2 = p1.stdout.readlines()
    for y in p2:
        y0,y1 = y.split('%s' % n)
        o[y0] = y1.strip('\n')

# m is a grass/bash command that will generate some list of info to stdout seperated by newlines, n is a defined blank list to write results to
def out2list(m, n):
        p1 = subprocess.Popen('%s' % m, stdout=subprocess.PIPE, shell='bash')
        p2 = p1.stdout.readlines()
        for y in p2:
            n.append(y.strip('\n'))
#SAVE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# m is a grass/bash command that will generate some info to stdout. You must invoke this command in the form of "variable to be made" = out2var('command')
def out2var(m):
        pn = subprocess.Popen('%s' % m, stdout=subprocess.PIPE, shell='bash')
        return pn.stdout.read()

# returns amoutn of free ramused like: var = getram()
def getram():
    if sys.platform == "win32":
        #mem1 = os.popen('mem | find "total"').readlines()
        mem = subprocess.Popen('mem | find "total"',  stdout=subprocess.PIPE)
        mem1 = mem.stdout.readlines()
        FreeMemory = int(mem1[0].split()[0])
    else:
        mem = subprocess.Popen('free -mo  | grep "Mem" | tr -s /[:blank:] /[:] | cut -d":" -f4')
        mem1 = mem.stdout.read()
        FreeMemory = int(mem1.strip('\n'))
    return FreeMemory
    
# now define  "main",  our main block of code, here defined because of the way g.parser needs to be called with python codes for grass (see below)        

# m = last iteration number, o = iteration number, p = prefx, q = statsout, r = resolution of input elev map
def main(m, o, p, q, r):
    # get variables from user input
    years = os.getenv("GIS_OPT_number")
    initbdrk = os.getenv("GIS_OPT_initbdrk")
    outbdrk = os.getenv("GIS_OPT_outbdrk")
    outdem = os.getenv("GIS_OPT_outdem")
    outsoil = os.getenv("GIS_OPT_outsoil")
    R = os.getenv("GIS_OPT_R")
    K = os.getenv("GIS_OPT_K")
    sdensity = os.getenv("GIS_OPT_sdensity")
    C = os.getenv("GIS_OPT_C")
    kappa = os.getenv("GIS_OPT_kappa")
    aplpha = os.getenv("GIS_OPT_method")
    #Ba = os.getenv("GIS_OPT_Ba")
    #Bb = os.getenv("GIS_OPT_Bb")
    sigma = os.getenv("GIS_OPT_sigma")
    nbhood = os.getenv("GIS_OPT_nbhood")
    cutoff1 = str(int(os.getenv("GIS_OPT_cutoff1")) / int(float(r) * float(r)))
    cutoff2 = str(int(os.getenv("GIS_OPT_cutoff2")) / int(float(r) * float(r)))
    cutoff3 = str(int(os.getenv("GIS_OPT_cutoff3")) / int(float(r) * float(r)))
    rain = os.getenv("GIS_OPT_rain")
    raindays = os.getenv("GIS_OPT_raindays")
    infilt = os.getenv("GIS_OPT_infilt")
    Kt = os.getenv("GIS_OPT_Kt")
    loadexp = os.getenv("GIS_OPT_loadexp")
    method = os.getenv("GIS_OPT_method")
    # make some variables for temporary map names
    flowacc = '%sflowacc%s' % (p, o)
    aspect = '%saspect%s' % (p, o)
    pc = '%spc%s' % (p, o)
    tc = '%stc%s' % (p, o)
    meancurv = '%smeancurv%s' % (p, o)
    rate = '%srate%s' % (p, o)
    sflowtopo = '%ssflowtopo%s' % (p, o)
    qsx = '%sqsx%s' % (p, o)
    qsy = '%sqsy%s' % (p, o)
    qsxdx = '%sqsx_dx%s' % (p, o)
    qsydy = '%sqsy_dy%s' % (p, o)
    erdep = '%serosdep%s' % (p, o)
    # make color rules for netchange maps
    nccolors = tempfile.NamedTemporaryFile()
    nccolors.write('100% 0 0 100\n1 blue\n0.5 indigo\n0.01 green\n0 white\n-0.01 yellow\n-0.5 orange\n-1 red\n0% 150 0 50')
    nccolors.flush()
    # make color rules for soil depth maps
    sdcolors = tempfile.NamedTemporaryFile()
    sdcolors.write('100% 0:249:47\n20% 78:151:211\n6% 194:84:171\n0% 227:174:217')
    sdcolors.flush()
    # if first iteration, use input maps. Otherwise, use maps generated from previous iterations
    if ( o == 1 ):
        old_dem = '%s' % os.getenv("GIS_OPT_elev")
        old_bdrk = '%s' % os.getenv("GIS_OPT_initbdrk")
        old_soil = "%s%s_init" % (prefx, os.getenv("GIS_OPT_outsoil"))
        grass.mapcalc('${out}=${m1}-${m2}', out = old_soil, m1 = old_dem, m2 = old_bdrk)
    else :
        old_dem = '%s%s%s' % (p, os.getenv("GIS_OPT_outdem"), m)
        old_bdrk = '%s%s%s' % (p, os.getenv("GIS_OPT_outbdrk"), m)
        old_soil = '%s%s%s' % (p, os.getenv("GIS_OPT_outsoil"), m)
    #checking for special condition of there being only one run, and setting variables accordingly (one year runs have no numbers suffixed to the output map names)
    if ( years == '1' ):
        slope = '%sslope' % p
        netchange = '%snetchange' % p
        new_dem ='%s%s' % (p, outdem)
        new_bdrk = '%s%s' % (p, outbdrk)
        new_soil = '%s%s' % (p, outsoil)
    else:
        slope = '%sslope%s' % (p, o)
        netchange = '%snetchange%s' % (p, o)
        new_dem = '%s%s%s' % (p, outdem, o)
        new_bdrk = '%s%s%s' % (p, outbdrk, o)
        new_soil = '%s%s%s' % (p, outsoil, o)
    grass.message('\n##################################################\n\n*************************\n Year %s ' % o + 'step 1 of 7: calculating slope and aspect\n*************************\n')
    grass.run_command('r.slope.aspect',  quiet = True, elevation = old_dem, slope = slope, aspect = aspect,  pcurv = pc, tcurv = tc)
    grass.message('\n*************************\n Year %s ' % o + 'step 2 of 7: calculating upslope accumulated flow\n*************************\n')       
    if ( os.getenv("GIS_FLAG_f") == "1" ):
        grass.message('using r.terraflow to calculate overland flow accumulation per cell (number of cells uplsope from each cell)\n\n')   
        #First we need to grab the amount of free RAM for r.terraflow
        mem = getram()
        #r.terraflow can't handle it if you tell it to use more than 2 Gigs of RAM, so if you have more than that, we have to tell r.terraflow to only use up to 2 Gigs of the free RAM... 
        if int(mem) > 2000:
            mem = '2000'    
        grass.message('Amount of free RAM being allocated for this step: %s Megabites' % mem)
        grass.run_command('r.terraflow',  quiet = True,  elev = old_dem, filled = p + '.filled', direction = p + '.direct', swatershed = p + '.sink', accumulation = flowacc, tci = p + '.tci',  d8cut = 'infinity', memory = mem, STREAM_DIR = os.path.expanduser('~'))
        grass.run_command('g.remove', quiet =True, rast = p + '.filled, ' + p + '.direct, ' + p + '.sink, ' + p + '.tci')
        os.remove(os.path.expanduser('~') + os.sep + 'STREAM*')
    else:
        grass.message('Using r.watershed to calculate overland flow accumulation per cell (number of cells uplsope from each cell)')
        if '<flag name="f">' in out2var('r.watershed --interface-description'):
            grass.run_command('r.watershed',  quiet = True,  flags = 'fa',  elevation = old_dem, accumulation = flowacc,  convergence = '5')
        else:
            grass.run_command('r.watershed',  quiet = True,  flags = 'a',  elevation = old_dem, accumulation = flowacc,  convergence = '5')

    grass.message('\n*************************\n Year %s ' % o + 'step 3 of 7: calculating basic sediment transport rates\n*************************\n')  
    error_message = '\n############################################################\n !!!!!!!!!YOU MUST SELECT ONLY ONE TYPE OF EROSION!!!!!!!!!!!\n ############################################################ \n \n'
    if ( os.getenv("GIS_FLAG_w") == "1" and  os.getenv("GIS_FLAG_d") == "0" and os.getenv("GIS_FLAG_r") == "0" ):
        grass.message('calculating for sheetwash across the entire map')
        grass.mapcalc('${sflowtopo} = ${flowacc} * sin(${slope})',  sflowtopo = sflowtopo,  flowacc = flowacc,  slope = slope)
    elif ( os.getenv("GIS_FLAG_w") == "0" and  os.getenv("GIS_FLAG_d") == "0" and os.getenv("GIS_FLAG_r") == "1" ):
        grass.message('calculating for channelzed flow across the entire map')
        grass.mapcalc('${sflowtopo} = exp(${flowacc}, 1.6) * exp(sin(${slope}), 1.3)',  sflowtopo = sflowtopo,  flowacc = flowacc,  slope = slope)
    elif ( os.getenv("GIS_FLAG_w") == "0" and  os.getenv("GIS_FLAG_d") == "1" and os.getenv("GIS_FLAG_r") == "0" ):
        grass.message('calculating for diffusive flow across the entire map')
        grass.mapcalc('${sflowtopo} = ${kappa} * exp((${r},2) * sin(${slope})',  out = sflowtopo,  kappa = kappa,  r = r,  slope = slope)
    elif ( os.getenv("GIS_FLAG_w") == "0" and  os.getenv("GIS_FLAG_d") == "1" and os.getenv("GIS_FLAG_r") == "1" ):
        sys.exit(error_message)
    elif ( os.getenv("GIS_FLAG_w") == "1" and  os.getenv("GIS_FLAG_d") == "0" and os.getenv("GIS_FLAG_r") == "1" ):
        sys.exit(error_message)
    elif ( os.getenv("GIS_FLAG_w") == "1" and  os.getenv("GIS_FLAG_d") == "1" and os.getenv("GIS_FLAG_r") == "0" ):
        sys.exit(error_message)
    elif ( os.getenv("GIS_FLAG_w") == "1" and  os.getenv("GIS_FLAG_d") == "1" and os.getenv("GIS_FLAG_r") == "1" ):
        sys.exit(error_message)
    else:
        # This step calculates the force of the flowing water at every cell on the landscape using the proper transport process law for the specific point in the flow regime. For upper hillslopes (below cutoff point 1) this done by multiplying the diffusion coeficient by the accumulated flow/cell res width. For midslopes (between cutoff 1 and 2) this is done by multiplying slope by accumulated flow with the m and n exponents set to 1. For channel catchment heads (between cutoff 2 and 3), this is done by multiplying slope by accumulated flow with the m and n exponents set to 1.6 and 1.3 respectively. For Channelized flow in streams (above cutoff 3), this is done by calculating the reach average shear stress (hydraulic radius [here estimated for a cellular landscape simply as the depth of flow]  times  slope times accumulated flow [cells] times gravitatiopnal acceleration of water [9806.65 newtons], all raised to the appropriate exponant for the type of transport (bedload or suspended load]). Depth of flow is claculated as a mean "instantaneous depth" during any given rain event, here assumed to be roughly equivelent tothe average depth of flow during 1 minute. 
        grass.mapcalc('${sflowtopo}= if(${flowacc} >= ${cutoff3}, exp((9806.65 * ((${rain} - (${rain} * ${infilt})) / (${raindays} * 1440)) * ${flowacc} * ${slope}), ${loadexp}), if(${flowacc} >= ${cutoff2} && ${flowacc} < ${cutoff3}, (exp((${flowacc}*${r}),1.6000000) * exp(sin(${slope}),1.3000000)), if(${flowacc} >= ${cutoff1} && ${flowacc} < ${cutoff2}, ((${flowacc}*${r}) * sin(${slope})), (${kappa} * sin(${slope}) ))))', sflowtopo = sflowtopo, rain = rain, infilt = infilt, raindays = raindays, flowacc = flowacc, cutoff1 = cutoff1,  cutoff2 = cutoff2,  cutoff3 = cutoff3,  slope = slope, kappa = kappa,  loadexp = loadexp,  r = r)
    
    grass.message('\n*************************\n Year %s ' % o + 'step 4 of 7: calculating sediment transport capacity in x and y directions\n*************************\n\n')
        #This step calculates the stream power or sediment carrying capacity (qs) of the water flowing at each part of the map by multiplying the reach average shear stress (channelized flow in streams) or the estimated flow force (overland flow) by the transport coeficient (estimated by R*K*C for hillslopes or kt for streams). This is a "Transport Limited" equation, however, we add some constraints on detachment by checking to see if the sediment supply has been exhausted: if the current soil depth is 0 or negative (checking for a negative value is kind of an error trap) then we make the transport coefficient small (0.000001) to simulate erosion on bedrock.
    grass.mapcalc('${qsx}=if(${old_soil} <= 0, (0.000001 * ${sflowtopo} * cos(${aspect})), if(${old_soil} > 0 && ${flowacc} >= ${cutoff3}, (${Kt} * ${sflowtopo} * cos(${aspect})), (${R} * ${K} * ${C} * ${sflowtopo} * cos(${aspect}))))', qsx = qsx, old_soil = old_soil, flowacc = flowacc, sflowtopo = sflowtopo, aspect = aspect, cutoff3 = cutoff3, Kt = Kt, R = R, K = K, C = C)
    grass.mapcalc('${qsy}=if(${old_soil} <= 0, (0.000001 * ${sflowtopo} * sin(${aspect})), if(${old_soil} > 0 && ${flowacc} >= ${cutoff3}, (${Kt} * ${sflowtopo} * sin(${aspect})), (${R} * ${K} * ${C} * ${sflowtopo} * sin(${aspect}))))', qsy = qsy, old_soil = old_soil, flowacc = flowacc, sflowtopo = sflowtopo, aspect = aspect, cutoff3 = cutoff3, Kt = Kt, R = R, K = K, C = C)


    grass.message('\n*************************\n Year %s ' % o + 'step 5 of 7: calculating partial derivatives for sediment transport\n*************************\n\n')
    grass.run_command('r.slope.aspect',  quiet = True,  elevation = qsx, dx = qsxdx)
    grass.run_command('r.slope.aspect',  quiet = True,  elevation = qsy, dy = qsydy)

    grass.message('\n*************************\n Year %s ' % o + 'step 6 of 7: calculating net erosion and deposition in meters of vertical change per cell\n*************************\n\n')
    #Then we add the x and y flux's for total flux "T" in Mg/Ha. Then if we multiply T times 100, we get a rate in grams/squaremeter. This new rate times the resolution gives us grams per cell width. Then we divide this by the density to get cubic centimeters per cell width. This measure is then divided by the area of the cell in centimeters (resolution squared x 100 squared) to get vertical change per cell width in centimeters. We dived this by 100 to get that measure in meters. Several of these facctors cancel out to make a final equation of "T/(10,000*density*resolution)". This equation changes the orignal T from Mg/Ha to vertical change in meters over the length of one cell's width.  In order to convert the output back to Mg/Ha (standard rate for USPED/RUSLE equations), you can multiply the netchange output map by "(10000 x resolution x soil density)" to create a map of soil erosion/deposition rates across the landscape. The rest of this mampcalc statement just checks the amount of erodable soil in a given cell against the amount of erosion calculated, and keeps the cell from eroding past this amount (if there is soil, then if the amount of erosion is more than the amount of soil, just remove all the soil and stop, else remove the amount of caclulated erosion. It also runs an error catch that checks to make sure that soil depth is not negative, and if it is, corrects it).
    grass.mapcalc('${erdep}= if( ${old_soil} >= 0, if( (-1 * ( ( (${qsxdx} + ${qsydy}) ) / ( 10000 * ${sdensity} ) ) ) >= ${old_soil}, ( -1 * ${old_soil} ), ( ( (${qsxdx} + ${qsydy}) )/ ( 10000 * ${sdensity} ) ) ), ( ( (${qsxdx} + ${qsydy}) ) / ( 10000 * ${sdensity} ) ) )', erdep = erdep, old_soil = old_soil, qsxdx = qsxdx, sdensity = sdensity, qsydy = qsydy)
    
    grass.message('\n*************************\n Year %s ' % o + 'step 7 of 7: calculating terrain evolution, new bedrock elevations, and new soil depths\n *************************\n\n')
    #put the net dz back where it is supposed to go (correct for the fact that, in grass, derivitaves of slope are calculated and stored one cell downslope from the cell where they actually belong) must be calulatedand then subtract it from dem to make new dem
    grass.mapcalc('${netchange} =eval(x=if((${aspect}  < 22.5 ||  ${aspect}  >= 337.5) && ${aspect}  != 0, (${erdep} [1,0]), if (${aspect}  >= 22.5 && ${aspect}  < 67.5, (${erdep} [1,-1]), if (${aspect}  >= 67.5 && ${aspect}  < 112.5, (${erdep} [0,-1]), if (${aspect}  >= 112.5 && ${aspect}  < 157.5, (${erdep} [-1,-1]), if (${aspect}  >= 157.5 && ${aspect}  < 202.5, (${erdep} [-1,0]), if (${aspect}  >= 202.5 && ${aspect}  < 247.5, (${erdep} [-1,1]), if (${aspect}  >= 247.5 && ${aspect}  < 292.5, (${erdep} [0,1]), if (${aspect}  >= 292.5 && ${aspect}  < 337.5, (${erdep} [1,1]), (${erdep} ))))))))), (if(isnull(x), ${erdep} , x)))', netchange = netchange, aspect = aspect, erdep = erdep)

    temp_dem = '%stemp_dem' % p
    grass.mapcalc('' + temp_dem + '=' + old_dem + ' + ' + netchange + '', temp_dem = temp_dem, old_dem = old_dem, netchange = netchange)
    #do patch-job to catch the shrinking edge problem
    grass.run_command('r.patch', quiet = True,  input = temp_dem + ',' + old_dem,  output= new_dem)
    #set colors for elevation map to match other dems
    grass.run_command('r.colors',  quiet = True, map = new_dem, rast = os.getenv("GIS_OPT_elev"))
    grass.run_command('g.remove', quiet = True, rast = temp_dem)
    #if asked, calculate amount of bedrock weathered and update soildepths map with this info, else just make a new soildepths map based on amount of erosion deposition
    if ( os.getenv("GIS_FLAG_b") == "1" ):
        grass.message('\nstep 8.5: Experimental bedrock weathering\n')
        grass.mapcalc('${meancurv}=((${pc} + ${tc}) / 2)', meancurv = meancurv, pc = pc,  tc = tc)
        # create dictionary to record max and min curvature
        statdict2 = grass.parse_command('r.info',  flags = 'r', map =meancurv)
        grass.message('\nThe raw max (' + statdict2['max'] + ') and min (' + statdict2['min'] + ') curvature will be rescaled from 2 to 0\n')
        # create map of bedrock weathering rates
        grass.mapcalc('${rate}=${kappa}*(2-(${meancurv}*(2/(${max})-(${min}))))', rate = rate, kappa = kappa, meancurv = meancurv,  max = statdict2['max]'], min = statdict2['min'])
        #rate is actually the net change in bedrock elevation due to soil production, so lets use it to find the new bedrock elev, and the new soil depth!
        grass.mapcalc('${new_bdrk}=${old_bdrk} - ${rate}', new_bdrk = new_bdrk, old_bdrk = old_bdrk, rate = rate)
        grass.message('Calculating new soil depths using new bedrock map')
        grass.mapcalc('${new_soil}=if ((${new_dem} - ${new_bdrk}) < 0, 0, (${new_dem} - ${new_bdrk}))',  new_soil = new_soil, new_dem = new_dem, new_bdrk = new_bdrk)
        #these are the old soil equations that I failed to be able to implement... I leave them in for documentation purposes
        #r.mapcalc "$new_bdrk=$initbdrk - ($Ba * ($Bb*($erdep - $initbdrk)))"
        #r.mapcalc "$new_soil=if (($erdep - $initbdrk) < 0, 0, ($erdep - $initbdrk))"
        grass.run_command('g.remove', quiet = True, rast = rate + ',' + meancurv)
    else:
        grass.message('\nCalculating new soil depths keeping bedrock elevations static\n')
        grass.mapcalc('${new_soil}=if ((${new_dem} - ${initbdrk}) < 0, 0, (${new_dem} - ${initbdrk}))', new_soil = new_soil, new_dem = new_dem, initbdrk = initbdrk)
    #setting colors for new soil depth map
    grass.message('reading color rules from %s' % sdcolors.name)
    grass.message (sdcolors.readlines())
    grass.run_command('r.colors', quiet = True, map = new_soil, rules = sdcolors.name)
    #make some temp maps of just erosion and just deposition so we can grab some stats from them
    tmperosion = p + 'tmperosion%s' % o
    tmpdep = p + 'tmpdep%s' % o
    grass.mapcalc('' + tmperosion + '=if(' + netchange + ' < 0, ' + netchange + ', null())', tmperosion = tmperosion, netchange = netchange)
    grass.mapcalc('' + tmpdep + '=if(' + netchange + ' > 0, ' + netchange + ', null())', tmpdep = tmpdep, netchange = netchange)

    #grab the stats fromt hese temp files and save them to dictionaries
    soilstats = grass.parse_command('r.univar', flags = 'ge', map = new_soil, percentile = '99')
    erosstats = grass.parse_command('r.univar', flags = 'ge', map = tmperosion, percentile = '1')
    depostats = grass.parse_command('r.univar', flags = 'ge', map = tmpdep,  percentile = '99')
    #clean up temp maps
    grass.run_command('g.remove',  quiet = True, rast = tmperosion + ',' + tmpdep)
    #write stats to a new line in the stats file
    grass.message('Outputing stats to textfile: ' + q)
    f.write('\n%s' % o + ',,' + erosstats['mean'] + ',' + erosstats['min'] + ',' + erosstats['max'] + ',' + erosstats['percentile_1'] + ',,' + depostats['mean'] + ',' + depostats['min'] + ',' + depostats['max'] + ',' + depostats['percentile_99'] + ',,' + soilstats['mean'] + ',' + soilstats['min'] + ',' + soilstats['max'] + ',' + soilstats['percentile_99'])
    #delete netchange map, if asked to, otherwise set colors for output netchange map
    if ( os.getenv("GIS_FLAG_n") == "1" ):
        grass.message('Not keeping a netchange map')
        grass.run_command('g.remove', quiet = True, rast = netchange)
    else:
        grass.run_command('r.colors', quiet = True, map = netchange, rules = nccolors.name)
    sdcolors.close()
    nccolors.close()
    grass.message('\n*************************\nDone with Year %s ' % o + '\n*************************\n')
    if ( m == '0' and os.getenv("GIS_FlAG_e") == "1" ):
        grass.message('\nkeeping initial soil depths map ' + old_soil + '\n')
    elif m == '0':
        grass.run_command('g.remove', quiet = True, rast = old_soil)
    grass.message('\nIf made, raster map ' + netchange + ' shows filtered net erosion/deposition\nIf made, raster map ' + new_soil + ' shows soildpeths\nRaster map ' + new_dem + ' shows new landscape (new elevations) after net erosion/depostion\n*************************\n')


# here is where the code in "main" actually gets executed. This way of programming is neccessary for the way g.parser needs to run.
if __name__ == "__main__":
    if ( len(sys.argv) <= 1 or sys.argv[1] != "@ARGS_PARSED@" ):
        os.execvp("g.parser", [sys.argv[0]] + sys.argv)
    else:
        # set up some basic variables
        years = os.getenv("GIS_OPT_number")
        prefx = os.getenv("GIS_OPT_prefx")
        #make the stats out file with correct column headers
        if os.getenv("GIS_OPT_statsout") == "":
            env = grass.gisenv()
            mapset = env['MAPSET']
            statsout = '%s_%s_lsevol_stats.txt' % (mapset, prefx)
        else:
            statsout = os.getenv("GIS_OPT_statsout")
        f = file(statsout, 'wt')
        f.write('Year,,Mean Erosion,Max Erosion,Min Erosion,99th Percentile Erosion,,Mean Deposition,Min Deposition,Max Deposition,99th Percentile Deposition,,Mean Soil Depth,Min Soil Depth,Max Soil Depth,99th Percentile Soil Depth')
        #region MUST be set to the map being used. otherwise r.flow will not work.
        #let's grab the current region
        region1 = grass.region()
        #now get map extents and resolution of input maps
        mapextent = grass.raster_info(os.getenv("GIS_OPT_elev"))
        #Now test to see if region extents and res matches map extents and res
        if (region1['e'] != mapextent['east'] or region1['w'] != mapextent['west'] or region1['n'] != mapextent['north'] or region1['s'] != mapextent['south'] or region1['nsres'] != mapextent['nsres'] or region1['ewres'] != mapextent['ewres'] ):
            grass.fatal('Region MUST be set to match input map: ' + os.getenv("GIS_OPT_elev") + '/n Please run g.region and then try again!')
        grass.message('\n##################################################\n##################################################\n\n STARTING SIMULATION\n\nBeginning iteration sequence. This may take some time.\nProcess is not finished until you see the message: \'Done with everything\'\n _____________________________________________________________\n_____________________________________________________________\n')
        grass.message("Total number of iterations to be run is %s years" % years)
        # This is the loop!
        for x in range(int(years)):
            grass.message("Iteration = %s" % (x + 1))
            main(x, (x + 1), prefx, statsout,  region1['nsres']);
        #since we are now done with the loop, close the stats file.
        f.close()
        grass.message('\nIterations complete, keeping region set to output maps\n')
        if os.getenv("GIS_FLAG_k") == "1":
            grass.message('\nTemporary maps have NOT been deleted\n')
        else:
            grass.message('\nCleaning up temporary maps...\n\n')
            if os.getenv("GIS_FLAG_p") == "0":
                grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'slope*')
            grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'flowacc*')
            grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'aspect*')
            grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'sflowtop*')
            grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'qsx*')
            grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'qsy*')
            grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'erosdep*')
            grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'pc*')
            grass.run_command('g.mremove', quiet =True, flags = 'f', rast = prefx + 'tc*')
            grass.message('\nDone\n\n')
        grass.message("\n\nDone with everything")




#!/usr/bin/env python
#
############################################################################
#
# MODULE:       r.bioclim
# AUTHOR(S):    Markus Metz
# PURPOSE:      Calculates bioclimatic indices from time series 
# COPYRIGHT:    (C) 2014 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Calcuates bioclimatic indices.
#% keywords: raster
#% keywords: time series
#% overwrite: yes
#%end
#%option G_OPT_R_INPUTS
#% key: tmin
#% label: Monthly minimum temperatures
#% description: Monthly minimum temperatures for 12 months
#% required: YES
#%end
#%option G_OPT_R_INPUTS
#% key: tmax
#% label: Monthly maximum temperatures
#% description: Monthly maximum temperatures for 12 months
#% required: YES
#%end
#%option G_OPT_R_INPUTS
#% key: tavg
#% label: Monthly average temperatures
#% description: Monthly average temperatures for 12 months
#% required: NO
#%end
#%option G_OPT_R_INPUTS
#% key: precipitation
#% label: Monthly precipitation
#% description: Monthly maximum temperatures for 12 months
#% required: NO
#%end
#%option
#% key: output
#% type: string
#% description: Prefix for output maps
#% required: YES
#%end
#%option
#% key: tinscale
#% type: integer
#% description: Scale factor for input temperature
#% answer: 1
#%end
#%option
#% key: toutscale
#% type: integer
#% description: Scale factor for output temperature
#% answer: 10
#%end
#%Option
#% key: workers
#% type: integer
#% required: no
#% multiple: no
#% options: 1-12
#% answer: 1
#% description: Number of parallel processes to launch
#%End

import sys
import os
import string
import grass.script as grass
import atexit

def cleanup():
    if tmp:
        grass.run_command('g.mremove', rast = tmp, flags = 'f', quiet = True)

def main():
    tavg = options['tavg']
    tmin = options['tmin']
    tmax = options['tmax']
    prec = options['precipitation']
    outpre = options['output']
    tinscale = options['tinscale']
    toutscale = options['toutscale']
    workers = int(options['workers'])

    mapset = grass.gisenv()['MAPSET']

    # count input maps
    if len(tmin.split(',')) != 12:
        grass.fatal(_("12 maps with minimum temperatures are required"))
    if len(tmax.split(',')) != 12:
        grass.fatal(_("12 maps with maximum temperatures are required"))
    if tavg and len(tavg.split(',')) != 12:
        grass.fatal(_("12 maps with average temperatures are required"))
    if prec:
        if len(prec.split(',')) != 12:
            grass.fatal(_("12 maps with precipitation are required"))

    tinscale = int(tinscale)
    if tinscale <= 0:
        grass.fatal(_("Input temperature scale must be positive"))
    toutscale = int(toutscale)
    if toutscale <= 0:
        grass.fatal(_("Output temperature scale must be positive"))

    pid = os.getpid()

    # all temporary raster maps must follow this naming pattern
    tmp = "%s.*.%d" % (outpre, pid)

    tminl = tmin.split(',')
    tmaxl = tmax.split(',')

    ps = {}

    if not tavg:
        # calculate monthly averages from min and max
        grass.message(_("Calculating monthly averages from min and max"))
        e = "$ta = ($tmax - $tmin) / 2.0"
        if workers > 1:
            for i in range(0, 12, workers):
                jend = 12 - i
                if jend > workers:
                    jend = workers
                
                for j in range(jend):
                    ta = "%s.tavg%02d.%d" % (outpre, (i + j + 1), pid)
                    ps[j] = grass.mapcalc_start(e, ta = ta, tmax = tmaxl[i + j], tmin = tminl[i + j])

                for j in range(jend):
                    ps[j].wait()
        else:
            for i in range(12):
                ta = "%s.tavg%02d.%d" % (outpre, (i + 1), pid)
                grass.mapcalc(e, ta = ta, tmax = tmaxl[i], tmin = tminl[i])

        tavg = grass.read_command("g.mlist",
                                 quiet = True,
                                 type = 'rast',
                                 pattern = '%s.tavg??.%d' % (outpre, pid),
                                 separator = ',',
                                 mapset= '.')
        tavg = tavg.strip('\n')

    tavgl = tavg.split(',')

    # BIO1 = Annual Mean Temperature
    grass.message(_("BIO1 = Annual Mean Temperature ..."))
    output = outpre + '.bio01.' + str(pid)
    grass.run_command('r.series', input = tavg, output = output, method = 'average')
    grass.mapcalc("$bio = round(double($oscale) * $input / $iscale)",
                  bio = outpre + '.bio01',
                  oscale = toutscale, input = output, iscale = tinscale)
    grass.run_command('g.remove', rast = output, quiet = True);
    
    # BIO2 = Mean Diurnal Range (Mean of monthly (max temp - min temp))
    grass.message(_("BIO2 = Mean Diurnal Range ..."))
    e = "$tr = $tmax - $tmin"
    if workers > 1:
        for i in range(0, 12, workers):
            jend = 12 - i
            if jend > workers:
                jend = workers
            
            for j in range(jend):
                tr = "%s.tr%02d.%d" % (outpre, (i + j + 1), pid)
                ps[j] = grass.mapcalc_start(e, tr = tr, tmax = tmaxl[i + j], tmin = tminl[i + j])

            for j in range(jend):
                ps[j].wait()

    else:
        for i in range(12):
            tr = "%s.tr%02d.%d" % (outpre, (i + 1), pid)
            grass.mapcalc(e, tr = tr, tmax = tmaxl[i], tmin = tminl[i])
    
    tr = grass.read_command("g.mlist",
                             quiet = True,
                             type = 'rast',
                             pattern = '%s.tr??.%d' % (outpre, pid),
                             separator = ',',
                             mapset= '.')

    tr = tr.strip('\n')

    output = outpre + '.bio02.' + str(pid)
    grass.run_command('r.series', input = tr, output = output, method = 'average')
    grass.mapcalc("$bio = round(double($oscale) * $input / $iscale)",
                  bio = outpre + '.bio02',
                  oscale = toutscale, input = output, iscale = tinscale)
    grass.run_command('g.remove', rast = output, quiet = True);
    grass.run_command('g.mremove', rast = '%s.tr??.%d' % (outpre, pid), flags = 'f', quiet = True)

    # BIO4 = Temperature Seasonality (standard deviation * 100)
    grass.message(_("BIO4 = Temperature Seasonality ..."))
    output = outpre + '.bio04.' + str(pid)
    grass.run_command('r.series', input = tavg, output = output, method = 'stddev')
    grass.mapcalc("$bio = round(100 * $biotmp / $iscale)",
                  bio = outpre + '.bio04',
                  biotmp = output,
                  iscale = tinscale)
    grass.run_command('g.remove', rast = output, quiet = True);

    # BIO5 = Max Temperature of Warmest Month
    grass.message(_("BIO5 = Max Temperature of Warmest Month ..."))
    output = outpre + '.bio05.' + str(pid)
    grass.run_command('r.series', input = tmax, output = output, method = 'maximum')
    grass.mapcalc("$bio = round(double($oscale) * $biotmp / $iscale)",
                  bio = outpre + '.bio05',
                  oscale = toutscale,
                  iscale = tinscale,
                  biotmp = output)
    grass.run_command('g.remove', rast = output, quiet = True);

    # BIO6 = Min Temperature of Coldest Month
    grass.message(_("BIO6 = Min Temperature of Coldest Month ..."))
    output = outpre + '.bio06.' + str(pid)
    grass.run_command('r.series', input = tmin, output = output, method = 'minimum')
    grass.mapcalc("$bio = round(double($oscale) * $biotmp / $iscale)",
                  bio = outpre + '.bio06',
                  oscale = toutscale,
                  biotmp = output,
                  iscale = tinscale)
    grass.run_command('g.remove', rast = output, quiet = True);

    # BIO7 = Temperature Annual Range (BIO5-BIO6)
    grass.message(_("BIO7 = Temperature Annual Range ..."))
    grass.mapcalc("$bio = $bio5 - $bio6",
                  bio = outpre + '.bio07',
                  bio5 = outpre + '.bio05',
                  bio6 = outpre + '.bio06')

    # BIO3 = Isothermality (BIO2/BIO7) (* 100)
    grass.message(_("BIO3 = Isothermality (BIO2/BIO7) ..."))
    grass.mapcalc("$bio = round(double($bio2) / $bio7 * 100)",
                  bio = outpre + '.bio03',
                  bio2 = outpre + '.bio02',
                  bio7 = outpre + '.bio07')

    # mean of mean for each quarter year
    grass.message(_("Mean temperature for each quarter year ..."))
    for i in range(0, 12, 3):
        tavgq = "%s.tavgq.%d.%d" % (outpre, i / 3, pid)

        grass.run_command('r.series', 
                           input = "%s,%s,%s" % (tavgl[i], tavgl[i + 1], tavgl[i + 2]),
                           output = tavgq, method = 'average')

    # BIO10 = Mean Temperature of Warmest Quarter
    # BIO11 = Mean Temperature of Coldest Quarter
    grass.message(_("BIO10 = Mean Temperature of Warmest Quarter,"))
    grass.message(_("BIO11 = Mean Temperature of Coldest Quarter ..."))

    tavgq = grass.read_command("g.mlist",
                             quiet = True,
                             type = 'rast',
                             pattern = '%s.tavgq.?.%d' % (outpre, pid),
                             separator = ',',
                             mapset= '.')

    tavgq = tavgq.strip("\n")
    bio10 = outpre + '.bio10.' + str(pid)
    bio11 = outpre + '.bio11.' + str(pid)
    grass.run_command('r.series', input = tavgq,
                      output = "%s,%s" % (bio10, bio11),
                      method = 'maximum,minimum')

    grass.mapcalc("$bio = round(double($oscale) * $biotmp / $iscale)",
                  bio = outpre + '.bio10',
                  oscale = toutscale,
                  biotmp = bio10,
                  iscale = tinscale)
    grass.mapcalc("$bio = round(double($oscale) * $biotmp / $iscale)",
                  bio = outpre + '.bio11',
                  oscale = toutscale,
                  biotmp = bio11,
                  iscale = tinscale)
    grass.run_command('g.remove', rast = "%s,%s" % (bio10, bio11), quiet = True);
    

    if not prec:
        grass.run_command('g.mremove', rast = tmp, flags = 'f', quiet = True)
        sys.exit(1)

    precl = prec.split(',')

    # sum for each quarter year
    grass.message(_("Precipitation for each quarter year ..."))
    for i in range(4):
        precq = "%s.precq.%d.%d" % (outpre, i + 1, pid)

        j = i * 3
        grass.run_command('r.series', 
                           input = "%s,%s,%s" % (precl[j], precl[j + 1], precl[j + 2]),
                           output = precq, method = 'sum')

    precq = grass.read_command("g.mlist",
                             quiet = True,
                             type = 'rast',
                             pattern = '%s.precq.?.%d' % (outpre, pid),
                             separator = ',',
                             mapset= '.')

    precq = precq.strip("\n")

    # warmest and coldest quarter
    warmestq = "%s.warmestq.%d" % (outpre, pid)
    coldestq = "%s.coldestq.%d" % (outpre, pid)
    grass.run_command('r.series', input = tavgq,
                      output = "%s,%s" % (warmestq, coldestq),
                      method = 'max_raster,min_raster')

    tavgql = tavgq.split(',')

    # wettest and driest quarter
    wettestq = "%s.wettestq.%d" % (outpre, pid)
    driestq = "%s.driestq.%d" % (outpre, pid)
    grass.run_command('r.series', input = precq,
                       output = "%s,%s" % (wettestq, driestq),
                       method = 'max_raster,min_raster')


    # BIO8 = Mean Temperature of Wettest Quarter
    grass.message(_("BIO8 = Mean Temperature of Wettest Quarter ..."))
    grass.mapcalc("$bio = round(graph($wettestq, \
                                      0, $tavgq0, \
                                      1, $tavgq1, \
                                      2, $tavgq2, \
                                      3, $tavgq3) * $oscale / $iscale)",
                  bio = outpre + '.bio08',
                  wettestq = wettestq, 
                  tavgq0 = tavgql[0], tavgq1 = tavgql[1],
                  tavgq2 = tavgql[2], tavgq3 = tavgql[3],
                  oscale = toutscale, iscale = tinscale)

    # BIO9 = Mean Temperature of Driest Quarter
    grass.message(_("BIO9 = Mean Temperature of Driest Quarter ..."))
    grass.mapcalc("$bio = round(graph($driestq, \
                                      0, $tavgq0, \
                                      1, $tavgq1, \
                                      2, $tavgq2, \
                                      3, $tavgq3) * $oscale / $iscale)",
                  bio = outpre + '.bio09',
                  driestq = driestq, 
                  tavgq0 = tavgql[0], tavgq1 = tavgql[1],
                  tavgq2 = tavgql[2], tavgq3 = tavgql[3],
                  oscale = toutscale, iscale = tinscale)

    # BIO12 = Annual Precipitation
    grass.message(_("BIO12 = Annual Precipitation ..."))
    output = outpre + '.bio12'
    grass.run_command('r.series', input = prec, output = output, method = 'sum')

    # BIO13 = Precipitation of Wettest Month
    # BIO14 = Precipitation of Driest Month
    grass.message(_("BIO13 = Precipitation of Wettest Month,"))
    grass.message(_("BIO14 = Precipitation of Driest Month ..."))

    bio13 = outpre + '.bio13'
    bio14 = outpre + '.bio14'
    grass.run_command('r.series', input = prec,
                      output = "%s,%s" % (bio13, bio14),
                      method = 'maximum,minimum')


    # BIO15 = Precipitation Seasonality (Coefficient of Variation)
    grass.message(_("BIO15 = Precipitation Seasonality ..."))
    precavg = "%s.precavg.%d" % (outpre, pid)
    precstddev = "%s.precstddev.%d" % (outpre, pid)
    grass.run_command('r.series', input = prec,
                      output = "%s,%s" % (precavg, precstddev),
                      method = 'average,stddev')
    grass.mapcalc("$bio = round(100.0 * $precstddev / $precavg)",
                  bio = outpre + '.bio15',
                  precstddev = precstddev,
                  precavg = precavg)

    # BIO16 = Precipitation of Wettest Quarter
    # BIO17 = Precipitation of Driest Quarter
    grass.message(_("BIO16 = Precipitation of Wettest Quarter,"))
    grass.message(_("BIO17 = Precipitation of Driest Quarter ..."))

    bio16 = outpre + '.bio16'
    bio17 = outpre + '.bio17'
    grass.run_command('r.series', input = precq,
                      output = "%s,%s" % (bio16, bio17),
                      method = 'maximum,minimum')

    precql = precq.split(',')

    # BIO18 = Precipitation of Warmest Quarter
    grass.message(_("BIO18 = Precipitation of Warmest Quarter ..."))

    grass.mapcalc("$bio = round(graph($warmestq, \
                                      0, $precq0, \
                                      1, $precq1, \
                                      2, $precq2, \
                                      3, $precq3))",
                  bio = outpre + '.bio18',
                  warmestq = warmestq, 
                  precq0 = precql[0], precq1 = precql[1],
                  precq2 = precql[2], precq3 = precql[3])

    # BIO19 = Precipitation of Coldest Quarter
    grass.message(_("BIO19 = Precipitation of Coldest Quarter ..."))

    grass.mapcalc("$bio = round(graph($coldestq, \
                                      0, $precq0, \
                                      1, $precq1, \
                                      2, $precq2, \
                                      3, $precq3))",
                  bio = outpre + '.bio18',
                  coldestq = coldestq, 
                  precq0 = precql[0], precq1 = precql[1],
                  precq2 = precql[2], precq3 = precql[3])


    grass.run_command('g.mremove', rast = tmp, flags = 'f', quiet = True)


if __name__ == "__main__":
    options, flags = grass.parser()
    tmp = None
    atexit.register(cleanup)
    main()

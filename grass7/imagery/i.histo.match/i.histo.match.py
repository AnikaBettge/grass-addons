#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       i.histo.match
# AUTHOR(S):    Luca Delucchi, Fondazione E. Mach (Italy)
#               original PERL code was developed by:
#               Laura Zampa (2004) student of Dipartimento di Informatica e 
#               Telecomunicazioni, Facoltà di Ingegneria,
#                University of Trento  and ITC-irst, Trento (Italy)
#
# PURPOSE:      Calculate histogram matching of several images
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################
#%module
#% description: Calculate histogram matching of several images.
#% keywords: raster
#%end
#%option G_OPT_R_INPUTS
#% description: Name of raster maps to analize
#% required: yes
#%end
#%option G_OPT_R_OUTPUT
#% description: Suffix for output maps
#% required: no
#% answer: match
#%end
#%option G_OPT_DB_DATABASE
#% required : no
#% answer: $GISDBASE/$LOCATION_NAME/$MAPSET/histo.db
#%end
#%option
#% key: max
#% type: integer
#% gisprompt: Number of the maximum value for raster maps
#% description: Number of the maximum value for raster maps
#% required: no
#% answer: 255
#%end


import sys, os, sqlite3
import grass.script as grass

def main():
    # split input images
    images = options['input'].split(',')
    # number of images
    n_images = len(images)
    # database path
    dbopt = options['database']
    # output suffix
    suffix = options['output']
    # name for average table
    table_ave = "t%s_average" % suffix
    # increment of one the maximum value for a correct use of range function
    max_value = int(options['max']) + 1
    # if the db path is the default one
    if dbopt.find('$GISDBASE/$LOCATION_NAME/$MAPSET') == 0:
        dbopt_split = dbopt.split(os.sep)[-1]
        env = grass.gisenv()
        path = os.path.join(env['GISDBASE'],env['LOCATION_NAME'],env['MAPSET'])
        dbpath = os.path.join(path,dbopt_split)
    else:
        if os.access(os.path.dirname(dbopt),os.W_OK):
            path = os.path.dirname(dbopt)
            dbpath = dbopt
        else:
            grass.fatal(_("Folder to write database files does not" \
                          + " exist or is not writeable"))
    # connect to the db
    db = sqlite3.connect(dbpath)
    curs = db.cursor()
    # for each image
    for i in images:
        # drop table if exist
        query_drop = "DROP TABLE if exists t%s" % i
        curs.execute(query_drop)
        # create table
        query_create = "CREATE TABLE t%s (grey_value integer,pixel_frequency " % i
        query_create += "integer, cumulative_histogram integer, cdf real)"
        curs.execute(query_create)
        # set the region on the raster
        grass.run_command('g.region', rast = i)
        # calculate statistics
        stats_out = grass.pipe_command('r.stats', flags='c', input= i, fs=':')
        stats =  stats_out.communicate()[0].split('\n')[:-1]
        stats_dict = dict( s.split(':', 1) for s in stats)
        cdf = 0
        # for each number in the range
        for n in range(0,max_value):
            # try to insert the values otherwise insert 0
            try:
                val = int(stats_dict[str(n)])
                cdf += val
                insert = "INSERT INTO t%s VALUES (%i, %i, %i, 0.000000)" % (i, n, val, cdf)
                curs.execute(insert)
            except:
                cdf += 0
                insert = "INSERT INTO t%s VALUES (%i, 0, %i, 0.000000)" % (i, n, cdf)
                curs.execute(insert)
        db.commit()
        # number of pixel is the cdf value
        numPixel = cdf
        # for each number in the range
        for n in range(0,max_value):
            # select value for cumulative_histogram for the range number
            select_ch="SELECT cumulative_histogram FROM t%s WHERE (grey_value=%i)" % (i, n)
            result = curs.execute(select_ch)
            val = result.fetchone()[0]
            # update cdf with new value
            if val != 0 and numPixel != 0:
                update_cdf = round(float(val) / float(numPixel),6)
                update_cdf = "UPDATE t%s SET cdf=%s WHERE (grey_value=%i)" % (i, update_cdf, n) 
                curs.execute(update_cdf)
                db.commit()
    db.commit()
    pixelTot = 0
    # for each number in the range
    for n in range(0,max_value):
        numPixel = 0
        # for each image
        for i in images:
            pixel_freq = "SELECT pixel_frequency FROM t%s WHERE (grey_value=%i)" % (i, n)
            result = curs.execute(pixel_freq)
            val = result.fetchone()[0]
            numPixel += val
        # calculate number of pixel divide by number of images
        div = (int(numPixel/n_images))
        pixelTot += div
    # drop average table
    query_drop = "DROP TABLE if exists %s" % table_ave
    curs.execute(query_drop)
    # create average table
    query_create = "CREATE TABLE %s (grey_value integer,average " % table_ave
    query_create += "integer, cumulative_histogram integer, cdf real)"
    curs.execute(query_create)
    cHist = 0
    # for each number in the range
    for n in range(0,max_value):
        tot = 0
        # for each image
        for i in images:
            # select pixel frequency
            pixel_freq = "SELECT pixel_frequency FROM t%s WHERE (grey_value=%i)" % (i, n)
            result = curs.execute(pixel_freq)
            val = result.fetchone()[0]
            tot += val
        # calculate new value of pixel_frequency
        average = (tot/n_images)  
        cHist = cHist + int(average)
        # insert new values into average table
        if cHist != 0 and pixelTot != 0:
            cdf = round(float(cHist) / float(pixelTot),6)
            insert = "INSERT INTO %s VALUES (%i, %i, %i, %s)" % (table_ave, n, int(average), cHist, cdf)
            curs.execute(insert)
            db.commit()
    # for each image
    for i in images:
        grass.run_command('g.region', rast = i)
        # write average rules file
        outfile = open(os.path.join(path,'%s.reclass' % i),'w')
        new_grey = 0
        for n in range(0,max_value):
            select_min = "SELECT min(abs(a.cdf - b.cdf)) FROM t%s as a," % i \
                        + " %s as b WHERE (a.grey_value=%i)" % (table_ave, n)
            result_min = curs.execute(select_min)
            min_abs = result_min.fetchone()[0]
            select_cdf = "SELECT cdf FROM t%s WHERE grey_value=%i" % (i, n)
            result_cdf = curs.execute(select_cdf)
            cdf = result_cdf.fetchone()[0]  
            select_newgrey = "SELECT grey_value FROM %s WHERE " % table_ave \
                + "cdf=(%s-%s) OR cdf=(%s+%s)" % (cdf,min_abs,cdf,min_abs)
            # write line with old and new value
            try:
                result_new = curs.execute(select_newgrey)
                new_grey = result_new.fetchone()[0] 
                out_line = "%d = %d\n" % (n, new_grey)
                outfile.write(out_line)                
            except:
                out_line = "%d = %d\n" % (n, new_grey)
                outfile.write(out_line)                 
        outfile.close() 
        outname = '%s.%s' % (i, suffix)
        # check if a output map already exists
        result = grass.core.find_file(outname, element = 'cell')
        if result['fullname'] and grass.overwrite():
            grass.run_command('g.remove', rast=outname)
            grass.run_command('r.reclass', input= i, out = outname, rules = outfile.name)
        elif result['fullname'] and not grass.overwrite():
            grass.warning(_("Raster map %s already exists and it will be not overwrite" % i))
        else:
            grass.run_command('r.reclass', input= i, out = outname, rules = outfile.name)
        # remove the rules file
        grass.try_remove(outfile.name)       
    db.commit()
    db.close()
    

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())



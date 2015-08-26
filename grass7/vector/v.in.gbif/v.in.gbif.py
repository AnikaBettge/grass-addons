#!/usr/bin/env python

"""
MODULE:    v.in.gbif

AUTHOR(S): Helmut Kudrnovsky <alectoria AT gmx at>

PURPOSE:   Imports GBIF species distribution data by saving original data to
           a GDAL VRT and importing the VRT by v.in.ogr

COPYRIGHT: (C) 2015 by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

#%module
#% description: importing of GBIF species distribution data
#% keyword: vector
#% keyword: geometry
#%end

#%option G_OPT_F_INPUT 
#% key: input
#% required: yes
#%end

#%option G_OPT_F_OUTPUT
#% key: output_vrt
#% description: VRT file (with vrt extension)
#% required: yes
#%end

#%option G_OPT_M_DIR
#% key: dir
#% description: Directory where the output will be found
#% required : yes
#%end

#%option G_OPT_V_OUTPUT
#% key: output
#% description: name of imported GBIF data set
#% required : yes
#%end

import sys
import os
import csv
import math
import grass.script as grass

if not os.environ.has_key("GISBASE"):
    grass.message( "You must be in GRASS GIS to run this program." )
    sys.exit(1)

def main():

    # check for unsupported locations
    in_proj = grass.parse_command('g.proj', flags='g')
    if in_proj['unit'].lower() == 'Meter':
        grass.fatal(_("Projected locations are not supported"))
    if in_proj['unit'].lower() == 'US survey foot':
        grass.fatal(_("Projected locations are not supported"))
    if in_proj['name'].lower() == 'xy_location_unprojected':
        grass.fatal(_("xy-locations are not supported"))

    gbifraw = options['input']
    directory = options['dir']
    gbifvrt = options['output_vrt']
    gbifimported = options['output']
    gbif_vrt_layer = options['output_vrt'].split('.')[0]
    gbifcsv = gbif_vrt_layer+'.csv'	
    global tmp	 
	
    # Extract vector line
    grass.message( "Starting importing GBIF data ..." )
    grass.message( "preparing data for vrt ..." )

    # new quoted GBIF csv file
    new_gbif_csv = os.path.join( directory, gbifcsv )

    # quote raw data
    with open('%s' % (gbifraw), 'rb') as csvinfile:
		gbifreader = csv.reader(csvinfile, delimiter='\t')
		with open ('%s' % (new_gbif_csv), 'wb') as csvoutfile:
			gbifwriter = csv.writer(csvoutfile, quotechar='"', quoting=csv.QUOTE_ALL)
			for row in gbifreader:
				gbifwriter.writerow(row)
    grass.message( "----" )

    # write	vrt		
    grass.message( "writing vrt ..." )
    new_gbif_vrt = os.path.join( directory, gbifvrt )
    
    f = open('%s' % (new_gbif_vrt), 'wt')
    f.write("""<OGRVRTDataSource>
    <OGRVRTLayer name="%s">
        <SrcDataSource relativeToVRT="1">%s</SrcDataSource>
        <GeometryType>wkbPoint</GeometryType> 
        <LayerSRS>WGS84</LayerSRS>
		<Field name="g_gbifid" src="gbifid" type="Integer" />
		<Field name="g_datasetkey" src="datasetkey" type="String" width="255" />
		<Field name="g_occurrenceid" src="occurrenceid" type="String" width="255" />
		<Field name="g_kingdom" src="kingdom" type="String" width="50" />
		<Field name="g_phylum" src="phylum" type="String" width="50" />
		<Field name="g_class" src="class" type="String" width="50" />
		<Field name="g_order" src="order" type="String" width="50" />
		<Field name="g_family" src="family" type="String" width="100" />
		<Field name="g_genus" src="genus" type="String" width="255" />
		<Field name="g_species" src="species" type="String" width="255" />
		<Field name="g_infraspecificepithet" src="infraspecificepithet" type="String" width="100" />
		<Field name="g_taxonrank" src="taxonrank" type="String" width="50" />
		<Field name="g_scientificname" src="scientificname" type="String" width="255" />
		<Field name="g_countrycode" src="countrycode" type="String" width="25" />
		<Field name="g_locality" src="locality" type="String" width="255" />
		<Field name="g_publishingorgkey" src="publishingorgkey" type="String" width="255" />
		<Field name="g_decimallatitude" src="decimallatitude" type="Real" />
		<Field name="g_decimallongitude" src="decimallongitude" type="Real" />
		<Field name="g_elevation" src="elevation" type="Real" />
		<Field name="g_elevationaccuracy" src="elevationaccuracy" type="String" width="50" />
		<Field name="g_depth" src="depth" type="String" width="255" />
		<Field name="g_depthaccuracy" src="depthaccuracy" type="String" width="255" />
		<Field name="g_eventdate" src="eventdate" type="String" width="255" />
		<Field name="g_day" src="day" type="Integer" width="255" />
		<Field name="g_month" src="month" type="Integer" width="255" />
		<Field name="g_year" src="year" type="Integer" width="255" />
		<Field name="g_taxonkey" src="taxonkey" type="String" width="100" />
		<Field name="g_specieskey" src="specieskey" type="String" width="100" />
		<Field name="g_basisofrecord" src="basisofrecord" type="String" width="100" />
		<Field name="g_institutioncode" src="institutioncode" type="String" width="100" />
		<Field name="g_collectioncode" src="collectioncode" type="String" width="100" />
		<Field name="g_catalognumber" src="catalognumber" type="String" width="255" />
		<Field name="g_recordnumber" src="recordnumber" type="String" width="255" />
		<Field name="g_identifiedby" src="identifiedby" type="String" width="255" />
		<Field name="g_rights" src="rights" type="String" width="255" />
		<Field name="g_rightsholder" src="rightsholder" type="String" width="255" />
		<Field name="g_recordedby" src="recordedby" type="String" width="255" />
		<Field name="g_typestatus" src="typestatus" type="String" width="255" />
		<Field name="g_establishmentmeans" src="establishmentmeans" type="String" width="255" />
		<Field name="g_lastinterpreted" src="lastinterpreted" type="String" width="255" />
		<Field name="g_mediatype" src="mediatype" type="String" width="100" />
		<Field name="g_issue" src="issue" type="String" width="255" />
		<GeometryField encoding="PointFromColumns" x="decimallongitude" y="decimallatitude"/>
	</OGRVRTLayer>	
	</OGRVRTDataSource>""" % (gbif_vrt_layer, gbifcsv) )
	
    f.close()

    grass.message( "----" )
    # Give information where output file are saved								 
    grass.message( "GBIF vrt files:" )
    grass.message( gbifvrt )
    grass.message( "-" )	
    grass.message( gbifcsv )
    grass.message( "are saved in:" )
    grass.message( directory )	
    grass.message( "----" )

    # import GBIF vrt
    grass.message( "importing GBIF vrt ..." )

    grass.run_command("v.in.ogr", input = new_gbif_vrt,
                                     layer = gbif_vrt_layer,
                                     output = gbifimported,
                                     quiet = True)

    grass.message( "..." )
    # v.in.gbif done!	
    grass.message( "importing GBIF data done!" )	

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())

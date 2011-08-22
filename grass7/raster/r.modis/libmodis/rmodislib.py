#!/usr/bin/env python
# -*- coding: utf-8 -*-

############################################################################
#
# MODULE:	r.modis
# AUTHOR(S):	Luca Delucchi
# PURPOSE:	Here there are some important class to run r.modis modules
#
#
# COPYRIGHT:	(C) 2011 by Luca Delucchi
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################
import grass.script as grass
# interface to g.proj -p
def get_proj():
    """!Returns the output from running "g.proj -p" plus towgs84 parameter (g.proj -d), 
    as a dictionary. Example:

    \code
    >>> proj = grass.get_proj()
    >>> (proj['name'], proj['ellps'], proj['datum'])
    (Lat/Lon, wgs84, wgs84)
    \endcode

    @return dictionary of projection values
    """
    gproj = grass.read_command('g.proj',flags='p')
    listproj = gproj.split('\n')
    listproj.remove('-PROJ_INFO-------------------------------------------------')
    listproj.remove('-PROJ_UNITS------------------------------------------------')
    listproj.remove('')
    proj = {}
    for i in listproj:
        ilist = i.split(':')
        proj[ilist[0].strip()] = ilist[1].strip()
    proj.update(grass.parse_command('g.proj',flags='j'))
    return proj

class product:
    """Definition of MODIS product with url and path in the ftp server
    """
    def __init__(self,value = None):
        # url to download products
        urlbase = 'e4ftl01u.ecs.nasa.gov'
        usrsnow = 'n4ftl01u.ecs.nasa.gov'
        ### values of lst product:
        lst_spec = '( 1 0 0 0 1 0 0 0 0 0 0 0 )'
        lst_specqa = '( 1 1 0 0 1 1 0 0 0 0 0 0 )'
        # pattern for r.bitpatter (key is the pattern option, list of values 
        #                         contain values for patval option)
        lst_patt = {3 : [2, 3], 128 : [81, 85], 255 : [129, 133, 145, 149]}
        # suffix for the lst product (key is the lst map, value the QA)
        lst1km_suff = {'.LST_Day_1km':'.QC_Day','.LST_Night_1km':'.QC_Night'}
        lst6km_suff = {'.LST_Day_6km':'.QC_Day','.LST_Night_6km':'.QC_Night'}
        # color for lst product
        lst_color = ['modis_lst']
        # all the possible layers of lst and vi products, it is used when mosaic it's created
        lstvi_specall = '( 1 1 1 1 1 1 1 1 1 1 1 1)'
        ### values of vi product:
        vi_spec = '( 1 1 0 0 0 0 0 0 0 0 0 )'
        vi_specqa = '( 1 1 1 0 0 0 0 0 0 0 0 )'
        vi_patt = {3 : [2, 3], 63 : [13, 14, 15], 128 : [3], 1024 : [1], 
        8192 : [0, 6, 7], 16384 : [1], 32768 : [1]}
        vi_color = ['ndvi','evi']
        vi_suff = {'.250m_16_days_NDVI' : '.250m_16_days_VI_Quality',
        '.250m_16_days_EVI' : '.250m_16_days_VI_Quality'}
        ### values of snow product:
        snow1_spec = ('( 1 )')
        snow1_specqa = ('( 1 1 )')
        snow1_specall = ('( 1 1 1 1 )')
        snow1_suff = {'.Snow_Cover_Daily_Tile':'.Snow_Spatial_QA'}
        snow1_patt = {3 : [2,3], 7 : [6, 7], 15 : [10, 11, 14, 15]}
        
        snow8_spec = ('( 1 1 )')
        snow_color = ['evi'] #TODO CREATE THE COLOR FOR MODIS_SNOW
        snow8_suff = {'.Maximum_Snow_Extent' : None, '.Eight_Day_Snow_Cover' : None}
        lstL2_spec = 'LST; QC; Error_LST; Emis_31; Emis_32; View_angle; View_time'
        
	self.prod = value
	lst = {'lst_aqua_daily_1000' : {'url' : urlbase, 'folder' : 'MOLA/MYD11A1.005',
                                  'res' : 1000, 'spec' : lst_spec, 'spec_qa' : lst_specqa,
                                  'spec_all' : lstvi_specall, 'suff' : lst1km_suff, 
                                  'pattern' : lst_patt, 'color' : lst_color
              },
              'lst_terra_daily_1000' : {'url' : urlbase, 'folder' : 'MOLT/MOD11A1.005',
                                  'res' : 1000, 'spec': lst_spec,'spec_qa': lst_specqa, 
                                  'spec_all' : lstvi_specall, 'suff' : lst1km_suff, 
                                  'pattern' : lst_patt, 'color' : lst_color
              }, 
              'lst_terra_eight_1000' : {'url' : urlbase, 'folder' : 'MOLT/MOD11A2.005',
                                  'res' : 1000, 'spec': lst_spec,'spec_qa': lst_specqa, 
                                  'spec_all' : lstvi_specall, 'suff' : lst1km_suff, 
                                  'pattern' : lst_patt, 'color' : lst_color
              },
              'lst_aqua_eight_1000' : {'url' : urlbase, 'folder' : 'MOLA/MYD11A2.005',
                                  'res' : 1000, 'spec': lst_spec,'spec_qa': lst_specqa, 
                                  'spec_all' : lstvi_specall, 'suff' : lst1km_suff, 
                                  'pattern' : lst_patt, 'color' : lst_color
              },
              'lst_terra_daily_6000' : {'url' : urlbase, 'folder' : 'MOLT/MOD11B1.005',
                                  'res' : 6000, 'spec': lst_spec,'spec_qa': lst_specqa, 
                                  'spec_all' : lstvi_specall, 'suff' : lst6km_suff, 
                                  'pattern' : lst_patt, 'color' : lst_color
              }, 
              'lst_aqua_daily_6000' : {'url' : urlbase, 'folder' : 'MOLA/MYD11B1.005',
                                  'res' : 6000, 'spec': lst_spec,'spec_qa': lst_specqa, 
                                  'spec_all' : lstvi_specall, 'suff' : lst6km_suff, 
                                  'pattern' : lst_patt, 'color' : lst_color
              },

        }
	vi = {'ndvi_terra_sixteen_250':{'url':urlbase, 'folder':'MOLT/MOD13Q1.005',
                                    'res':250,'spec': vi_spec,'spec_qa': vi_specqa,
                                    'spec_all' : lstvi_specall, 'suff' : vi_suff, 
                                    'pattern' : vi_patt, 'color' : vi_color
              },
              'ndvi_aqua_sixteen_250':{'url':urlbase, 'folder':'MOLA/MYD13Q1.005',
                                    'res':250,'spec': vi_spec,'spec_qa': vi_specqa,
                                    'spec_all' : lstvi_specall, 'suff' : vi_suff, 
                                    'pattern' : vi_patt, 'color' : vi_color
              },
              'ndvi_terra_sixteen_500':{'url':urlbase, 'folder':'MOLT/MOD13A2.005',
                                    'res':500,'spec': vi_spec,'spec_qa': vi_specqa,
                                    'spec_all' : lstvi_specall, 'suff' : vi_suff, 
                                    'pattern' : vi_patt, 'color' : vi_color
              },
              'ndvi_aqua_sixteen_500':{'url':urlbase, 'folder':'MOLA/MYD13A2.005',
                                    'res':500,'spec': vi_spec,'spec_qa': vi_specqa,
                                    'spec_all' : lstvi_specall, 'suff' : vi_suff, 
                                    'pattern' : vi_patt, 'color' : vi_color
              }
        }
        snow = {'snow_terra_daily_500' : {'url' : usrsnow, 'folder' : 
				'SAN/MOST/MOD10A1.005', 'res' : 500, 'spec' : snow1_spec
				,'spec_qa': snow1_specqa,'spec_all' :snow1_specall ,
				'suff' : snow1_suff, 'pattern' : snow1_patt, 
				'color' : snow_color
		}, 
		'snow_aqua_daily_500' : {'url' : usrsnow, 'folder' : 
                                'SAN/MOSA/MYD10A1.005', 'res' : 500, 'spec' : snow1_spec
                                ,'spec_qa': snow1_specqa, 'spec_all' : snow1_specall,
                                'suff' : snow1_suff, 'pattern' : snow1_patt, 
                                'color' : snow_color
                },
                'snow_terra_eight_500' : {'url' : usrsnow, 'folder' : 
                                'SAN/MOST/MOD10A2.005', 'res' : 500, 'spec' : snow8_spec
                                ,'spec_qa': None, 'spec_all' : snow8_spec, 
                                'pattern' : None, 'suff' : snow8_suff, 'color' : snow_color
                }, 
                'snow_aqua_eight_500' : {'url' : usrsnow, 'folder' : 
                                'SAN/MOSA/MYD10A2.005', 'res' : 500, 'spec' : snow8_spec
                                ,'spec_qa': None, 'spec_all' : snow8_spec, 
                                'pattern' : None, 'suff' : snow8_suff, 'color' : snow_color
                }
	}
	self.products = { }
	self.products.update(lst)
	self.products.update(vi)
	self.products.update(snow)
        self.products_swath = { 
          'lst_terra_daily':{'url':urlbase,'folder':'MOLT/MOD11_L2.005',
          'spec': lstL2_spec}, 'lst_aqua_daily':{'url':urlbase,'folder':'MOLA/MYD11_L2.005',
          'spec': lstL2_spec}
        }
    def returned(self):
        if self.products.keys().count(self.prod) == 1:
            return self.products[self.prod]
        elif self.products_swath.keys().count(self.prod) == 1:
            return self.products_swath[self.prod]
        else:
            grass.fatal(_("The code insert is not supported yet. Consider to ask " \
                      + "on the grass-dev mailing list for future support"))

    def fromcode(self,code):
        import string
        for k,v in self.products.iteritems():
          if string.find(v['folder'],code) != -1:
            return self.products[k]
        for k,v in self.products_swath.iteritems():
          if string.find(v['folder'],code) != -1:
            return self.products_swath[k]
        grass.fatal(_("The code insert is not supported yet. Consider to ask " \
                      "on the grass-dev mailing list for future support"))

    def color(self,code = None):
        if code:
          return self.fromcode(code)['color']
        else:
          return self.returned()['color']


    def pattern(self,code = None):
        if code:
          return self.fromcode(code)['pattern']
        else:
          return self.returned()['pattern']

    def suffix(self,code = None):
        if code:
          return self.fromcode(code)['suff']
        else:
          return self.returned()['suff']

    def __str__(self):
	prod = self.returned()
	string = "url: " + prod['url'] + ", folder: " + prod['folder']
        if prod.keys().count('spec') == 1:
                string += ", spectral subset: " + prod['spec']
        if prod.keys().count('spec_qa') == 1:
                string += ", spectral subset qa:" + prod['spec_qa']
	return string

class resampling:
    """Return the resampling value from the code used in the modules
    """
    def __init__(self,value):
        self.code = value
        self.resampling = {'NN': 'NEAREST_NEIGHBOR','BI' : 'BICUBIC',
        'CC' : 'CUBIC CONVOLUTION','NONE' : 'NONE'}

    def returned(self):
        return self.resampling[self.code]

class projection:
    """Definition of projection for converting from sinusoidal projection to
    another one. Not all projection systems are supported"""
    def __init__(self):
        self.proj = get_proj()
        self.val = self.proj['proj']
        self.dat = self.proj['datum']
        self.projections = {'laea' : 'LAMBERT AZIMUTHAL EQUAL AREA','ll':'GEO', 
             'lcc':'LAMBERT CONFORMAL CONIC', 'merc':'MERCATOR', 
             'polar':'POLAR STEREOGRAPHIC', 'utm':'UTM', 
             'tmerc':'TRANSVERSE MERCATOR'}
        self.datumlist = {'none':'NONE', 'nad27':'NAD27', 'nad83':'NAD83', 
        'wgs66':'WGS66', 'wgs72':'WGS72', 'wgs84':'WGS84', 'etrs89':'WGS84'}
        self.datumlist_swath = {'Clarke 1866' : 0, 'Clarke 1880' : 1, 'bessel' : 2
            , 'International 1967' : 3, 'International 1909': 4, 'wgs72' : 5, 
            'Everest' : 6, 'wgs66' : 7, 'wgs84' : 8, 'Airy' : 9, 
            'Modified Everest' : 10, 'Modified Airy' : 11, 'Walbeck' : 12, 
            'Southeast Asia' : 13, 'Australian National' : 14, 'Krassovsky' : 15, 
            'Hough' : 16, 'Mercury1960' : 17, 'Modified Mercury1968' : 18, 
            'Sphere 19 (Radius 6370997)' : 19, 'MODIS Sphere (Radius 6371007.181)' : 20}

    def returned(self):
        """Return the projection in the MRT style"""
        if self.val not in self.projections.keys():
            grass.fatal(_("Projection <%s> is not supported") % self.val)
        else:
            return self.projections[self.val]

    def _par(self,key):
        """Function use in return_params"""
        if self.proj[key]:
            SMinor = self.proj[key]
        else:
            SMinor = 0.0

    def _outpar(self, SMajor, SMinor, Val, Factor, CentMer, TrueScale, FE, FN,swath):
        if swath:
          return '%i %i %d %d %d %d %d %d 0.0 0.0 0.0 0.0 0.0 0.0 0.0' % ( 
                SMajor, SMinor, Val, Factor, CentMer, TrueScale, FE, FN )
        else:
          return '( %i %i %d %d %d %d %d %d 0.0 0.0 0.0 0.0 0.0 0.0 0.0 )' % ( 
                SMajor, SMinor, Val, Factor, CentMer, TrueScale, FE, FN )

    def return_params(self, swath = False):
        """ Return the 13 paramaters for parameter file """
        if self.val == 'll' or self.val == 'utm':
            return self._outpar(0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, swath)
        elif self.val == 'lcc':
            SMajor = self._par('+a')
            if self._par('+b'):
                SMinor = self._par('+b')
            elif self._par('+rf'):
                SMinor = self._par('+rf')
            STDPR1 = self._par('+lat_1')
            STDPR2 = self._par('+lat_2')
            CentMer = self._par('+lon_0')
            CentLat = self._par('+lat_0')
            FE = self._par('+x_0')
            FN = self._par('+y_0')
            return self._outpar(SMajor, SMinor, STDPR1, STDPR2, CentMer, 
                                CentLat, FE, FN, swath)
        elif self.val == 'merc' or self.val == 'polar' or self.val == 'tmerc':
            SMajor = self._par('+a')
            if self._par('+b'):
                SMinor = self._par('+b')
            elif self._par('+rf'):
                SMinor = self._par('+rf')
            CentMer = self._par('+lon_0')
            if self.val == 'tmerc':
                Factor = self._par('+k_0')
            else:
                Factor = 0.0
            TrueScale = self._par('+lat_ts')
            FE = self._par('+x_0')
            FN = self._par('+y_0')
            return self._outpar(SMajor, SMinor, 0.0, Factor, CentMer, 
                                 TrueScale, FE, FN, swath)
        else:
            grass.fatal(_('Projection not supported, please contact the' \
                          'GRASS-dev mailing list'))

    def datum(self):
        """Return the datum in the MRT style"""
        if self.dat is not in self.datumlist.keys():
            grass.fatal(_("Datum <%s> is not supported") % self.dat)
        elif self.dat == 'etrs89':
            grass.warning(_("Changing etrs89 to wgs84"))
        return self.datumlist[self.dat]

    def datumswath(self):
        """Return the datum in MRT style"""
        return self.datumlist_swath[self.dat]

    def utmzone(self):
        """Return the utm zone number"""
        return self.proj['zone']

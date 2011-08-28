"""!
@package parse.py

@brief Python app for parsing the getCapabilties reponse, checking
service exception in the wms server reply and hierarchical display
of layers.

Functions:
 - parsexml
 - isValidResponse
 - isServiceException
 - populateLayerTree
 - dfs

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author: Maris Nartiss (maris.nartiss gmail.com)
@author Sudeep Singh Walia (Indian Institute of Technology, Kharagpur , sudeep495@gmail.com)
"""

from grass.script import core as grass
from BeautifulSoup import BeautifulSoup, BeautifulStoneSoup
import re
from xml.dom.minidom import parse, parseString
from urllib2 import Request, urlopen, URLError, HTTPError


key = 0

class newLayerData():
	name = None
	title = None
	abstract = None
	srsList = None

class LayerData():
	name = None
	title = None
	abstract = None
	srs = None
	
	def printLayerData(self,layerDataDict):
		for key, value in layerDataDict.iteritems():
			print key
			print value.name.string
			print value.title.string
			print value.abstract.string
			srss = value.srs
			for srs in srss:
				a = srs.string
				a = a.split(':')
				print a[0]+' '+a[1]
			print '--------------------------------------------'
	def appendLayerTree(self, layerDataDict, LayerTree, layerTreeRoot):
		for key, value in layerDataDict.iteritems():
			name = value.name.string
			title = value.title.string
			abstract = value.abstract.string
			string = str(key)+"-"+name+":"+title+":"+abstract
			LayerTree.AppendItem(layerTreeRoot, string)


def parsexml(xml):

 xmltext = xml
 soup = BeautifulSoup(xmltext)
 #layers = soup.findAll('layer', queryable="1")
 layers = soup.findAll('layer')
 
 namelist = []
 for layer in layers:
	soupname = BeautifulSoup(str(layer))
	names =  soupname.findAll('name')
	if len(names) > 0:
		namelist += names[0]
 return namelist


def parsexml2(xml):
 layerDataDict={}
 count = -1
 xmltext = xml
 soup = BeautifulSoup(xmltext)
 layers = soup.findAll('layer')
 namelist = []
 for layer in layers:
	soupname = BeautifulSoup(str(layer))
	names =  soupname.findAll('name')
	titles = soupname.findAll('title')
	abstracts = soupname.findAll('abstract')
	srs = soupname.findAll('srs')
	if(len(names)>0):
		count = count + 1
		layerDataDict[count] = LayerData()
		layerDataDict[count].name = unicode(names[0].string)
	else:
		continue
	if(len(titles)>0):
		layerDataDict[count].title = unicode(titles[0].string)
	else:
		layerDataDict[count].title = ''

	if(len(abstracts)>0):
		layerDataDict[count].abstract = unicode(abstracts[0].string)
	else:
		layerDataDict[count].abstract = ''

	if(len(srs)>0):
		layerDataDict[count].srs = srs
	else:
		layerDataDict[count].srs = ''
	

 return layerDataDict



def isValidResponse(xml):
	soup = BeautifulSoup(xml)
	getCapabilities = soup.findAll('wmt_ms_capabilities')
	if(len(getCapabilities)==0):
		return False
	else:
		return True
def isServiceException(xml):
	soup = BeautifulSoup(xml)
	exceptions = soup.findAll('ServiceException')
	exceptionList = []
	xmltext = str(xml)
	if(xmltext.count('ServiceException') > 0):
		return True
	else:
		return False

def populateLayerTree(xml,LayerTree, layerTreeRoot):
	TMP = grass.tempfile()
	if TMP is None:
		grass.fatal("Unable to create temporary files")
	f = open(TMP,'w')
	f.write(xml)
	f.close()
	f = open(TMP,'r')
	xml = f.read()
	soup = BeautifulSoup(xml)
	dfs(soup,LayerTree, layerTreeRoot)

def test(xml,LayerTree,layerTreeRoot):
	f = open('/home/sudeep/in3.xml','w')
	f.write(xml)
	f.close()
	f = open('/home/sudeep/in3.xml','r')
	xml1=f.read()
	'''
	#xml1=xml
	#xml='<root> '+xml1+' </root>'
	a=xml1.find('<WMT_MS_Capabilities')
	print 'a1='+str(a)
	if(a==-1):
		print 'a2='+str(a)
		a=xml1.find('<wmt_ms_capabilities')
	if(a==-1):
		print 'a3='+str(a)
		print 'serious mix up'
		return
	print 'a4='+str(a)
	#print xml1[a:]
	'''
	dom=parseString(xml1)
	root=dom.firstChild
	lData = {}
	global key
	key = 0
	dfs1(dom,LayerTree,layerTreeRoot,lData)
	return lData

	
def dfs(root,LayerTree, ltr):
	
	if not hasattr(root, 'contents'):
		print root.string
		return
	else:
		id = ltr
		print root.name
		if(root.name == 'layer'):
			names = root.findAll('name')
			if(len(names)>0):
				id = LayerTree.AppendItem(ltr,names[0].string)
				print  names[0].string
		children = root.contents
		for child in children:
			dfs(child, LayerTree, id)
    		return

def getAttributeLayers(node, attribute):
	Attribute = attribute.capitalize() 
	l=node.getElementsByTagName(attribute)
	g=None
	if(len(l)>0):
		g=l[0].firstChild
	else:
		l=node.getElementsByTagName(Attribute)
		if(len(l)>0):
			g=l[0].firstChild
	if(g is not None):
		return unicode(g.nodeValue)
	else:
		return None
	    
def dfs1(node,LayerTree, ltr,lData):
	global key
	if ( hasattr(node,'data')):
		return
	id = ltr
	if(hasattr(node,'tagName')):
		if(node.tagName == 'Layer' or node.tagName == 'layer'):
		   	name = getAttributeLayers(node, 'name')
		   	if(name is not None):
		 		lData[str(key)] = newLayerData()
		   		title = getAttributeLayers(node, 'title')
		   		abstract = getAttributeLayers(node, 'abstract')
		   		if(title is None):
		   			title = unicode('')
		   		else:
		   			title = ':'+title
		   			
		   		if(abstract is None):
		   			abstract = unicode('')
		   		else:
		   			abstract = ':'+abstract
		   			
		   		description = unicode(str(key)+'-'+name+title+abstract)
		   		id = LayerTree.AppendItem(ltr,description)
		   		
		   		SRS = node.getElementsByTagName('SRS')
		   		srsList = []
		   		for srs in SRS:
		   			#print srs.toxml()
		   			#print srs.firstChild.nodeValue
		   			srsList += [str(srs.firstChild.nodeValue)[5:]]
		   		print srsList
		   		lData[str(key)].name = name
		   		lData[str(key)].abstract = abstract
		   		lData[str(key)].title = title
		   		lData[str(key)].srsList = srsList
		   		key = key + 1
		   			
		   		
	for child in node.childNodes:
		dfs1(child,LayerTree,id,lData)
	return
	
	
	
def parseGrass_Region(grassRegion, dir):
	grassRegion = 'n-s resol: 26.266417; n-s resol3: 100; rows: 533; north: 4928000.0; t-b resol: 1; zone: 13; bottom: 0; rows3: 140; west: 590000.0; top: 1; cols: 698; cols3: 190; depths: 1; e-w resol: 27.220630; proj: 1; e-w resol3: 100; east: 609000.0; south: 4914000.0;' 
	width = '698'
	Height = '533'
	s = grassRegion.find(dir)
	g = grassRegion[s:]
	g = g.split()
	g = g[1].strip(';')
	return float(g)

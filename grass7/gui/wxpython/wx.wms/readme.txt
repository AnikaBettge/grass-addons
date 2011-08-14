This is a GSOC Project to add WMS GetCapabilites functionality support for Grass.

Aim of the project-

This project aims to introduce several features that make WXGUI more user-friendly. These include adding WMS layer support for WXGUI. The selection features will be displayed as per the based on service GetCapabilities response. The parameters and the layers are to be displayed are extracted out of the xml response.

State of the art-

The project uses WMS service to fetch images of the maps. The Get Feature service of WMS services provides a GML based interface to access the information about various layers available. The WMS layer knowledge is used to update the GUI of the GRASS according to the available features for a particular layer. Thw WX-GUI is to be modified and to be integrated with the WMS services. 

Libraries to be installed

1.  BeautifulSoup 3.2 , an XML, HTML Parser ( http://www.crummy.com/software/BeautifulSoup/ )


How to run the add-on

1) Create a directory wms in as grass_trunk/gui/wxpython/gui_modules/wms. Place all the files in the svn root/grass-addons/grass7/gui/wxpython/wx.wmswms directory in the created wms directory. 
2) Change the path for config file in the file LoadConfig.py as per the where config file is placed. (This need is temporary, will be resolved soon)
3) Patch wxgui.py and toolbars.py with command "cd path/to/sourceroot && cat core.diff | patch -p0"
4) make the source code
5) Now run it , when launched , a new button shall appear in Main GUI window beside 'Start New Map Display'
6) Click on it, and a new wms window shall be launched. 


Bugs:

Known bugs are in TODO file

Author - 
Sudeep Singh Walia

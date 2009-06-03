#!/usr/bin/python
# -*- coding:utf-8 -*-
"""
MODULE:    v.autokrige2

AUTHOR(S): Anne Ghisla <a.ghisla AT gmail.com>

PURPOSE:   Performs ordinary kriging

DEPENDS:  R (version?), package automap (geoR? gstat?)

COPYRIGHT: (C) 2009 by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

#list of parameters

#import directives. to be completed.
import wx
import sys
#import wx.lib.flatnotebook as FN # self.notebook attribute in Kriging Module's __init__

#classes in alphabetical order

class KrigingModule(wx.Frame):
    """
    Kriging module for GRASS GIS.
    """
    def __init__(self, parent, *args, **kwargs):
        wx.Frame.__init__(self, parent, *args, **kwargs)
        # setting properties and all widgettery
        self.SetTitle("Kriging Module")
        self.log = Log(self) # writes on statusbar
        self.CreateStatusBar()
        # debug: remove before flight
        self.log.write("Are you reading this? This proves it is very beta version.")
        
        # sizer stuff. Improvable.
        vsizer1 = wx.BoxSizer(orient=wx.VERTICAL)
        # uncommment this line when panels will be ready to be added
#        vsizer1.Add(item=ITEM, proportion=1, flag=wx.EXPAND | wx.ALL, border=10)
        self.SetSizerAndFit(vsizer1)
#        self.SetAutoLayout(True)
#        vsizer1.Fit(self)
        #@TODO(anne): set minimum size around objects.
        
#        global variables. don't know if it is a good place to store them, in case move them
        
#        widgettery. They are all Panels into StaticBox Sizers.
#
#        1. Combobox with all point layers of the mapset with xyz data: see how to call d.vect.
#               whateverNameOfFunction(ltype='vector',
#                                            lname=name,
#                                            lchecked=True,
#                                            lopacity=1.0,
#                                            lcmd=['d.vect', 'map=%s' % name])
#        2. Variogram.
#            RadioButton (default choice): Auto-fit Variogram
#            RadioButton: Choose variogram parameters -> Button that opens a dialog with the variogram plot and controls.
#        3. Kriging.
#            2 Notebook pages, one for R package gstat and other for geoR. Each page will have the available options, zB:
#            RadioButton (default): Ordinary Kriging
#            RadioButton: Block Kriging
#            RadioButton: Cokriging
#        n. OK - Cancel buttons
        
    
class Log:
    """
    The log output is redirected to the status bar of the containing frame.
    """
    def __init__(self, parent):
        self.parent = parent

    def write(self, text_string):
        """Update status bar"""
        self.parent.SetStatusText(text_string.strip())

#main
def main(argv=None):
    if argv is None:
        argv = sys.argv

    """if len(argv) != 2:
        print >> sys.stderr, __doc__
        sys.exit()"""

    # Command line arguments of the script to be run are preserved by the
    # hotswap.py wrapper but hotswap.py and its options are removed that
    # sys.argv looks as if no wrapper was present.
    #print "argv:", `argv`

    # commmented, as I don't know if the module will need it.
    ##some applications might require image handlers
    #wx.InitAllImageHandlers()

#    app = wx.PySimpleApp() # deprecated?
    app = wx.App()
    k = KrigingModule(parent=None, size=(600,300))
    k.Show()
    app.MainLoop()

if __name__ == '__main__':
    main()

#
## notebook stuff. If used, uncomment also import directive for flat notebook.
#        self.notebook = FN.FlatNotebook(parent=self.panel, id=wx.ID_ANY,
#                                        style=FN.FNB_BOTTOM |
#                                        FN.FNB_NO_NAV_BUTTONS |
#                                        FN.FNB_FANCY_TABS | FN.FNB_NO_X_BUTTON)
#
#        # use this block to add pages to the flat notebook widget        
##        dbmStyle = globalvar.FNPageStyle
#        ## start block
#        self.collectDataPage = FN.FlatNotebook(self.panel, id=wx.ID_ANY)
#        self.notebook.AddPage(self.collectDataPage, text=("Collect Data"))
##        self.browsePage.SetTabAreaColour(globalvar.FNPageColor)
#        ## end block
#    def __createCollectDataPage(self):
#        pass

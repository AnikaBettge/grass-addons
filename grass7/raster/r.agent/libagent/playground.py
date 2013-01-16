"""
MODULE:       r.agent.*
AUTHOR(S):    michael lustenberger inofix.ch
PURPOSE:      library file for the r.agent.* suite
COPYRIGHT:    (C) 2011 by Michael Lustenberger and the GRASS Development Team

              This program is free software under the GNU General Public
              License (>=v2). Read the file COPYING that comes with GRASS
              for details.
"""

class Playground(object):
    """A Playground is a major component of a World, defining
       and organizing space."""
    def __init__(self):
        self.layers = dict()
        self.region = dict(n=0,s=0,w=0,e=0)
    def getregion(self):
        """
        Return the region information
        @return dict region
        """
        return self.region
    def getbound(self, bound):
        pass
    def setlayer(self, layername, layer, force=False):
        pass
    def createlayer(self, layername, grassmap=False):
        pass
    def getlayer(self, layername):
        return []
    def removelayer(self, layername):
        pass
    def writelayer(self, layername):
        pass


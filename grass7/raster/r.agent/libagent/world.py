"""
MODULE:       r.agent.*
AUTHOR(S):    michael lustenberger inofix.ch
PURPOSE:      library file for the r.agent.* suite
COPYRIGHT:    (C) 2011 by Michael Lustenberger and the GRASS Development Team

              This program is free software under the GNU General Public
              License (>=v2). Read the file COPYING that comes with GRASS
              for details.
"""

import error
import playground
import agent

class World(object):
    """Generic World class as basis for more complex worlds."""

    def __init__(self, agenttype=None, pg=None, freedom=8):
        """
        Create a World, a place with a playground, where agents meet
        @param type optional, the default agent type for this world
        @param playground optional, if playground already exists
        """
        if pg == None:
            self.playground = playground.Playground()
        else:
            self.playground = pg
        if agenttype == None:
            self.agenttype = agent.Agent
        else:
            self.agenttype = agenttype
        self.agents = []
        #self.artefacts = []
        self.agentfreedom = freedom

    def addlayertopg(self, layername):
        """
        Add a new layer to the playground
        @param string name for the new layer
        """
        self.playground.createlayer(layername)

    def removelayerfrompg(self, layername):
        """
        Remove the named layer from the playground
        @param string name of the layer to be removed
        """
        self.playground.removelayer(layername)

    def getlayer(self, layername):
        """
        Access a layer by its name
        @param string name of the layer
        @return the layer
        """
        return self.playground.getlayer(layername)

    def findposition(self, position=None):
        """
        Find a given position on the playground, i.e. test if the
        given position is valid or invent a new one by creating a
        random one
        @param list optional coordinates on the playground or None for random
        @return list position or False if the given one was invalid
        """
        if position:
            if self.playground.isvalidposition(position):
                return position
            else:
                return False
        else:              
            return self.playground.getrandomposition()

    def bear(self, timetolive, position=None, agenttype=None):
        """
        Set a new agent into the world
        @param int number of cycles the agent has to live
        @param list coordinates to put the agent or none for a random position
        @return agent the newly created agent
        """
        position = self.findposition(position)
        if not position:
            raise error.DataError("r.agent::libagent.world.World.bear()",
                                  "invalid position")
        agent = self.agenttype(timetolive, self, position)
        self.agents.append(agent)
        return agent

    def move(self, agent, position=None):
        """
        Set agent to a new position
        @param agent to be moved
        @param list coordinates of the new position or none for a random one
        """
        position = self.findposition(position)
        if not position:
            raise error.DataError("r.agent::libagent.world.World.move()",
                                  "invalid position")
        agent.setposition(position)

    def getneighbourpositions(self, position, freedom=None):
        """
        Get all the positions reachable
        @param list coordinates of a certain cell
        @param int number of potentially reachable neighbours
        @return list list of coordinates
        """
        if not freedom:
            freedom = self.agentfreedom
        return self.playground.getneighbourpositions(position, freedom)

    def kill(self, agent):
        """
        Remove the agent from the list
        @param agent to be terminated
        """
        self.agents.remove(agent)


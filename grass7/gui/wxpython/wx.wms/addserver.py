#!/usr/bin/env python
# -*- coding: utf-8 -*-
# generated by wxGlade 0.6.3 on Thu Jul 14 06:22:35 2011

import wx

# begin wxGlade: extracode
# end wxGlade



class ServerAdd(wx.Frame):
    def __init__(self, *args, **kwds):
        # begin wxGlade: ServerAdd.__init__
        kwds["style"] = wx.DEFAULT_FRAME_STYLE
        wx.Frame.__init__(self, *args, **kwds)
        self.Servers = wx.StaticText(self, -1, "Servers")
        self.combo_box_1 = wx.ComboBox(self, -1, choices=[], style=wx.CB_DROPDOWN)
        self.static_line_1 = wx.StaticLine(self, -1)
        self.ServerName = wx.StaticText(self, -1, "ServerName")
        self.ServerNameText = wx.TextCtrl(self, -1, "")
        self.URL = wx.StaticText(self, -1, "URL")
        self.URLText = wx.TextCtrl(self, -1, "")
        self.Username = wx.StaticText(self, -1, "Username")
        self.UsernameText = wx.TextCtrl(self, -1, "")
        self.Password = wx.StaticText(self, -1, "Password")
        self.PasswordText = wx.TextCtrl(self, -1, "")
        self.static_line_2 = wx.StaticLine(self, -1)
        self.Save = wx.Button(self, -1, "Save")
        self.Remove = wx.Button(self, -1, "Remove")
        self.AddNew = wx.Button(self, -1, "AddNew")
        self.Quit = wx.Button(self, -1, "Quit")

        self.__set_properties()
        self.__do_layout()
        # end wxGlade

    def __set_properties(self):
        # begin wxGlade: ServerAdd.__set_properties
        self.SetTitle("frame_2")
        self.Servers.SetMinSize((90, 17))
        self.ServerName.SetMinSize((90, 20))
        self.ServerNameText.SetMinSize((189, 25))
        self.URL.SetMinSize((90, 20))
        self.URLText.SetMinSize((189, 25))
        self.Username.SetMinSize((90, 20))
        self.UsernameText.SetMinSize((189, 25))
        self.Password.SetMinSize((90, 20))
        self.PasswordText.SetMinSize((189, 25))
        # end wxGlade

    def __do_layout(self):
        # begin wxGlade: ServerAdd.__do_layout
        sizer_1 = wx.BoxSizer(wx.VERTICAL)
        sizer_7 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_6 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_5 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_4 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_3 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_2 = wx.BoxSizer(wx.HORIZONTAL)
        sizer_2.Add(self.Servers, 0, 0, 0)
        sizer_2.Add(self.combo_box_1, 0, 0, 0)
        sizer_1.Add(sizer_2, 0, wx.EXPAND, 0)
        sizer_1.Add(self.static_line_1, 0, wx.EXPAND, 0)
        sizer_3.Add(self.ServerName, 0, 0, 0)
        sizer_3.Add(self.ServerNameText, 0, 0, 0)
        sizer_1.Add(sizer_3, 1, wx.EXPAND, 0)
        sizer_4.Add(self.URL, 0, 0, 0)
        sizer_4.Add(self.URLText, 0, 0, 0)
        sizer_1.Add(sizer_4, 1, wx.EXPAND, 0)
        sizer_5.Add(self.Username, 0, 0, 0)
        sizer_5.Add(self.UsernameText, 0, 0, 0)
        sizer_1.Add(sizer_5, 1, wx.EXPAND, 0)
        sizer_6.Add(self.Password, 0, 0, 0)
        sizer_6.Add(self.PasswordText, 0, 0, 0)
        sizer_1.Add(sizer_6, 1, wx.EXPAND, 0)
        sizer_1.Add(self.static_line_2, 0, wx.EXPAND, 0)
        sizer_7.Add(self.Save, 0, 0, 0)
        sizer_7.Add(self.Remove, 0, 0, 0)
        sizer_7.Add(self.AddNew, 0, 0, 0)
        sizer_7.Add(self.Quit, 0, 0, 0)
        sizer_1.Add(sizer_7, 1, wx.EXPAND, 0)
        self.SetSizer(sizer_1)
        sizer_1.Fit(self)
        self.Layout()
        # end wxGlade

# end of class ServerAdd


if __name__ == "__main__":
    app = wx.PySimpleApp(0)
    wx.InitAllImageHandlers()
    frame_2 = ServerAdd(None, -1, "")
    app.SetTopWindow(frame_2)
    frame_2.Show()
    app.MainLoop()

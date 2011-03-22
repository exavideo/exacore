# Copyright 2011 Andrew H. Armenia.
# 
# This file is part of openreplay.
# 
# openreplay is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# openreplay is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with openreplay.  If not, see <http://www.gnu.org/licenses/>.

import wx
import socket
import sys

class Server:
    def __init__(self, host):
        for res in socket.getaddrinfo(host, 30005, socket.AF_UNSPEC, socket.SOCK_STREAM):
            af, socktype, proto, canonname, sa = res
            try:
                s = socket.socket(af, socktype, proto)
            except socket.error, msg:
                s = None
                continue

            try:
                s.connect(sa)
            except socket.error, msg:
                s.close()
                s = None
                continue

            break

        if s is None:
            print "failed to open socket"
            sys.exit(1)

        self.s = s

    def request(self, req):
        self.s.send(req + "\n")
        reply = self.s.recv(1024)


class MyFrame(wx.Frame):
    def mkbutton(self, text, func):
        bt = wx.Button(self, -1, text)
        self.Bind(wx.EVT_BUTTON, func, bt)
        self.sz.Add(bt, 1, wx.EXPAND)

    def __init__(self, server):
        wx.Frame.__init__(self, None, -1, "Scoreboard Control")
        self.server = server

        # create a grid sizer
        self.sz = wx.GridSizer(0, 4, 2, 2)

        # create and bind buttons
        self.mkbutton("Dissolve In", self.dissolve_in)
        self.mkbutton("Dissolve Out", self.dissolve_out)
        self.mkbutton("Home +", self.home_goal)
        self.mkbutton("Away +", self.away_goal)
        self.mkbutton("Delayed Penalty", self.delayed_penalty)
        self.mkbutton("Full Strength", self.full_strength)
        self.mkbutton("RPI Goal", self.rpi_goal)
        self.mkbutton("#21 Polacek", self.polacek)
        self.mkbutton("Text Out", self.text_out)
        self.mkbutton("Clock Start", self.clock_start)
        self.mkbutton("Clock Stop", self.clock_stop)

        self.SetSizer(self.sz)
        self.SetAutoLayout(1)
    
    def dissolve_in(self, evt):
        self.server.request("dissolve_in")

    def dissolve_out(self, evt):
        self.server.request("dissolve_out")

    def home_goal(self, evt):
        self.server.request("home_goal")

    def away_goal(self, evt):
        self.server.request("away_goal")

    def delayed_penalty(self, evt):
        self.server.request("text_up #dfd800 #000000 DELAYED PENALTY")

    def full_strength(self, evt):
        self.server.request("text_up #666666 #ffffff FULL STRENGTH")

    def rpi_goal(self, evt):
        self.server.request("text_up #666666 #ffffff RPI GOAL")

    def polacek(self, evt):
        self.server.request("text_up #666666 #ffffff G: #21 POLACEK")

    def text_out(self, evt):
        self.server.request("text_down")

    def clock_start(self, evt):
        self.server.request("start_clock")
    
    def clock_stop(self, evt):
        self.server.request("stop_clock")

s = Server('localhost')
app = wx.App()
f = MyFrame(s)
f.Show(1)
app.MainLoop()

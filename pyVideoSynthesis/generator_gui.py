import wx #sudo apt install python-wxgtk3.0-dev
from generator import Generator
from textctrl import DefaultTextCtrl
from dialog import Dialog, yuvwildcard, cfgwildcard


class VideoSynthesisFrame(wx.Frame):

    def __init__(self, parent = None, title = 'VideoSynthesisGenerator', size = (700, 400)):
        self.gen = Generator()
        wx.Frame.__init__(self, parent = parent, title = title, size = size)
        self.alignToCenter()

        self.bkg = wx.Panel(self)
        self.__createComponents()
        self.__setRegions()

        self.Show()

    def __createComponents(self):
        #---path buttons---#
        self.loadtextureButton = wx.Button(self.bkg, label='Open Texture Video')
        self.Bind(wx.EVT_BUTTON, self.OnTextureButton, self.loadtextureButton)

        self.loaddepthButton = wx.Button(self.bkg, label = 'Open Depth Video')
        self.Bind(wx.EVT_BUTTON, self.OnDepthButton, self.loaddepthButton)

        #self.loadcamBotton = wx.Button(self.bkg, label = 'Open Camera params')
        #self.Bind(wx.EVT_BUTTON, self.OnCamButton, self.loadcamBotton)

        self.saveButton = wx.Button(self.bkg, label = 'Save Video Dir')
        self.Bind(wx.EVT_BUTTON, self.OnSaveButton, self.saveButton)

        #---path texts---#
        self.texture = DefaultTextCtrl(self.bkg, 'Please select the Texture YUV(4:2:0) File.', style=wx.TE_READONLY)
        self.depth = DefaultTextCtrl(self.bkg, 'Please select the Depth YUV(4:2:0) File.', style=wx.TE_READONLY)
        #self.camera = DefaultTextCtrl(self.bkg, 'Please select the Camera parameters File.', style=wx.TE_READONLY)
        self.save = DefaultTextCtrl(self.bkg, 'Please select the save directory.', style=wx.TE_READONLY)

        #---views texts---#
        self.totalviews = DefaultTextCtrl(self.bkg, str(self.gen.totalViews), isInt = True)
        self.leftviews = DefaultTextCtrl(self.bkg, str(self.gen.leftViews), isInt = True)
        self.rightviews = DefaultTextCtrl(self.bkg, str(self.gen.rightViews), isInt = True)
        self.viewdistance = DefaultTextCtrl(self.bkg, str(self.gen.viewDistant), isFloat = True)

        #---input texts---#
        self.width = DefaultTextCtrl(self.bkg, str(self.gen.width), isInt = True)
        self.height = DefaultTextCtrl(self.bkg, str(self.gen.height), isInt = True)
        self.frames = DefaultTextCtrl(self.bkg, str(self.gen.frames), isInt = True)
        self.skips = DefaultTextCtrl(self.bkg, str(self.gen.skips), isInt = True)

        #---cams texts---#
        self.focallength = DefaultTextCtrl(self.bkg, str(self.gen.FocalLen), isFloat = True)
        self.cshift = DefaultTextCtrl(self.bkg, str(self.gen.CShift), isFloat = True)
        self.znear = DefaultTextCtrl(self.bkg, str(self.gen.ZNear), isFloat = True)
        self.zfar = DefaultTextCtrl(self.bkg, str(self.gen.ZFar), isFloat = True)

        #---start button---#
        self.startButton = wx.Button(self.bkg, label = 'START>>', size = (100, 200))
        self.Bind(wx.EVT_BUTTON, self.OnStart, self.startButton)

#-----------------Actions-----------------------------------#
    def OnTextureButton(self, evt):
        Dialog.openFileDialog(self, self.texture, yuvwildcard)

    def OnDepthButton(self, evt):
        Dialog.openFileDialog(self, self.depth, yuvwildcard)

    def OnCamButton(self, evt):
        Dialog.openFileDialog(self, self.camera, cfgwildcard)

    def OnSaveButton(self, evt):
        Dialog.saveDirDialog(self, self.save)

    def OnStart(self, evt):
        # 1. Check parameter
        self.gen.textureFile = self.texture.GetValue()
        print 'hello ' + self.gen.textureFile + '\n'
        self.gen.depthFile = self.depth.GetValue()
        self.gen.cameraFile = self.camera.GetValue()

        self.gen.totalViews = int(self.totalviews.GetValue())
        self.leftViews = int(self.leftviews.GetValue())
        self.rightViews = int(self.rightviews.GetValue())
        self.viewDistant = float(self.viewdistance.GetValue())

        self.gen.width = int(self.width.GetValue())
        self.gen.height = int(self.height.GetValue())
        self.gen.frames = int(self.frames.GetValue())
        self.gen.skips = int(self.skips.GetValue())

        self.gen.FocalLen = float(self.focallength.GetValue())
        self.gen.CShift = float(self.cshift.GetValue())
        self.gen.ZNear = float(self.znear.GetValue())
        self.gen.ZFar = float(self.zfar.GetValue())
        dlg = wx.ProgressDialog("Progress dialog example",
                               "An informative message",
                               maximum = 100,
                               parent=self.bkg,
                               style = wx.PD_APP_MODAL
                                | wx.PD_CAN_ABORT
                                )
        #if not self.gen.check():
        self.gen.run()
        dlg.Destroy()
        #self.gauge.Disconnect()
        # 2. Start gererating.
        print 'TODO:'
#---------------------------------------------------------#

#------------------Layout---------------------------------#
    def alignToCenter(self):
        dw, dh = wx.DisplaySize()
        w, h = self.GetSize()
        x = (dw - w)/2
        y = (dh - h)/2
        self.SetPosition((x, y))


    def __setRegions(self):
        hbox = wx.BoxSizer()
        vbox0 = self.__fileRegion()
        hbox0 = self.__viewPointsRegion()
        hbox1 = self.__inputParametersRegion()
        hbox2 = self.__camParameterRegion()
        hbox3 = self.__startRegion()
        hbox.Add(hbox0, proportion = 0 ,flag=wx.EXPAND | wx.ALL, border = 5)
        hbox.Add(hbox1, proportion = 0 ,flag=wx.EXPAND | wx.ALL, border = 5)
        hbox.Add(hbox2, proportion = 0, flag=wx.EXPAND | wx.ALL, border = 5)
        hbox.Add(hbox3, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)
        vbox = wx.BoxSizer(wx.VERTICAL)
        vbox.Add(vbox0, proportion = 0, flag=wx.EXPAND | wx.ALL, border=5)
        vbox.Add(hbox, proportion = 0, flag=wx.EXPAND | wx.ALL, border=5)
        self.bkg.SetSizer(vbox)

    def __textGroup(self, staticText, valueText):
        hbox = wx.BoxSizer()
        hbox.Add(staticText, proportion=0, flag=wx.CENTER, border=5)
        hbox.Add(valueText, proportion=0, flag=wx.CENTER, border=5)
        return hbox

    def __fileRegion(self):
        #TODO: Need to refine the BOX
        hbox0 = wx.BoxSizer()
        hbox1 = wx.BoxSizer()
        hbox2 = wx.BoxSizer()
        hbox3 = wx.BoxSizer()

        hbox0.Add(self.loadtextureButton, proportion = 0, flag=wx.CENTER, border=5)
        hbox0.Add(self.texture, proportion=1, flag=wx.EXPAND)

        hbox1.Add(self.loaddepthButton, proportion = 0, flag=wx.CENTER | wx.ALIGN_TOP, border=5)
        hbox1.Add(self.depth, proportion=1, flag=wx.EXPAND | wx.ALIGN_TOP)

        #hbox2.Add(self.loadcamBotton, proportion = 0, flag=wx.CENTER | wx.ALIGN_TOP, border=5)
        #hbox2.Add(self.camera, proportion=1, flag=wx.EXPAND | wx.ALIGN_TOP)

        hbox3.Add(self.saveButton, proportion = 0, flag=wx.CENTER | wx.ALIGN_TOP, border=5)
        hbox3.Add(self.save, proportion=1, flag=wx.EXPAND | wx.ALIGN_TOP)

        vbox = wx.BoxSizer(wx.VERTICAL)
        vbox.Add(hbox0, proportion = 0, flag=wx.EXPAND | wx.ALL, border=5)
        vbox.Add(hbox1, proportion = 0, flag=wx.EXPAND | wx.ALL, border=5)
        #vbox.Add(hbox2, proportion = 0, flag=wx.EXPAND | wx.ALL, border=5)
        vbox.Add(hbox3, proportion = 0, flag=wx.EXPAND | wx.ALL, border=5)

        return vbox


    def __viewPointsRegion(self):
        box = wx.StaticBox(self.bkg, -1, 'ViewPoints')
        boxer = wx.StaticBoxSizer(box, wx.VERTICAL)

        l1 = wx.StaticText(self.bkg, -1, "TotalNum:")
        l2 = wx.StaticText(self.bkg, -1, "LeftNum:")
        l3 = wx.StaticText(self.bkg, -1, "RightNum:")
        l4 = wx.StaticText(self.bkg, -1, "ViewDist:")

        boxer.Add(self.__textGroup(l1, self.totalviews), 0, wx.ALL | wx.EXPAND, 10)
        boxer.Add(self.__textGroup(l2, self.leftviews), 0, wx.ALL | wx.EXPAND, 10)
        boxer.Add(self.__textGroup(l3, self.rightviews), 0, wx.ALL | wx.EXPAND, 10)
        boxer.Add(self.__textGroup(l4, self.viewdistance), 0, wx.ALL | wx.EXPAND, 10)
        return boxer

    def __inputParametersRegion(self):
        box = wx.StaticBox(self.bkg, -1, 'InputParams')
        boxer = wx.StaticBoxSizer(box, wx.VERTICAL)

        l1 = wx.StaticText(self.bkg, -1, "Width:")
        l2 = wx.StaticText(self.bkg, -1, "Height:")
        l3 = wx.StaticText(self.bkg, -1, "TotalFrames:")
        l4 = wx.StaticText(self.bkg, -1, "SkipFrames:")

        boxer.Add(self.__textGroup(l1, self.width), 0, wx.ALL | wx.EXPAND, 10)
        boxer.Add(self.__textGroup(l2, self.height), 0, wx.ALL | wx.EXPAND, 10)
        boxer.Add(self.__textGroup(l3, self.frames), 0, wx.ALL | wx.EXPAND, 10)
        boxer.Add(self.__textGroup(l4, self.skips), 0, wx.ALL | wx.EXPAND, 10)
        return boxer

    def __camParameterRegion(self):
        box = wx.StaticBox(self.bkg, -1, 'CamParameters')
        boxer = wx.StaticBoxSizer(box, wx.VERTICAL)

        l1 = wx.StaticText(self.bkg, -1, "FocalLen:")
        l2 = wx.StaticText(self.bkg, -1, "CShift:")
        l3 = wx.StaticText(self.bkg, -1, "ZNear:")
        l4 = wx.StaticText(self.bkg, -1, "ZFar:")

        boxer.Add(self.__textGroup(l1, self.focallength), 0, wx.ALL | wx.CENTER, 10)
        boxer.Add(self.__textGroup(l2, self.cshift), 0, wx.ALL | wx.CENTER, 10)
        boxer.Add(self.__textGroup(l3, self.znear), 0, wx.ALL | wx.CENTER, 10)
        boxer.Add(self.__textGroup(l4, self.zfar), 0, wx.ALL | wx.CENTER, 10)
        return boxer

    def __startRegion(self):
        hbox= wx.BoxSizer()
        hbox.Add(self.startButton, proportion=1, flag=wx.EXPAND, border=10)
        return hbox
#--------------------------------------------------------------------------------------#

def main():
    app = wx.App()
    VideoSynthesisFrame()
    app.MainLoop()

if __name__ == '__main__':
    main()

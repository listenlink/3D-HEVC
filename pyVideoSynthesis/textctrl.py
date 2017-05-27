import wx

class DefaultTextCtrl(wx.TextCtrl):
    def __init__(self, parent = None, text='', style=wx.TE_RICH, isFloat=False, isInt=False):
        wx.TextCtrl.__init__(self, parent = parent, id = -1,  value = text, style = wx.TE_RICH | style)
        self.SetForegroundColour(wx.LIGHT_GREY)
        self.SetBackgroundColour(wx.WHITE)
        self.Bind(wx.EVT_TEXT, self.EvtText)
        self.isFloat = isFloat
        self.isInt = isInt

    def EvtText(self, event):
        input = event.GetString()
        print('EvtText: %s\n' % input)
        if(self.isFloat):
            text = float(input)
            self.changeColor()
        if(self.isInt):
            text = int(input)
            self.changeColor()

    def changeText(self, text):
        self.SetValue(value = text)
        self.changeColor()

    def changeColor(self):
        self.SetForegroundColour(wx.BLACK)
        self.SetBackgroundColour(wx.GREEN)


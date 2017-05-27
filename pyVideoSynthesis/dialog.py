import os
import wx

#---------------------------------------------------------------------------
yuvwildcard = "YUV(4:2:0) Files (*.yuv)|*.yuv|"     \
           "All files (*.*)|*.*"

cfgwildcard = "Cfg Files (*.cfg)|*.cfg|"     \
           "All files (*.*)|*.*"

#---------------------------------------------------------------------------


class Dialog:

    @staticmethod
    def openFileDialog(parent, contextCtrl, wildcard):
        print("CWD: %s\n" % os.getcwd())

        dlg = wx.FileDialog(
            parent, message="Choose a file",
            defaultDir=os.getcwd(),
            defaultFile="",
            wildcard=wildcard,
            style=wx.OPEN | wx.MULTIPLE | wx.CHANGE_DIR
            )

        if dlg.ShowModal() == wx.ID_OK:
            # This returns a Python list of files that were selected.
            paths = dlg.GetPaths()
            contextCtrl.changeText(paths[0])
            print('You selected %d files:' % len(paths))
            for path in paths:
                print('           %s\n' % path)

        # Compare this with the debug above; did we change working dirs?
        print("CWD: %s\n" % os.getcwd())

        # Destroy the dialog. Don't do this until you are done with it!
        # BAD things can happen otherwise!
        dlg.Destroy()

    @staticmethod
    def saveDirDialog(parent, contextCtrl):
        # In this case we include a "New directory" button.
        dlg = wx.DirDialog(parent, "Choose a directory:",
                          style=wx.DD_DEFAULT_STYLE)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            print('You selected: %s\n' % path)
            contextCtrl.changeText(path)

        # Only destroy a dialog after you're done with it.
        dlg.Destroy()

from enum import Enum, unique #sudo apt install python-enum34
from os import system, path

dir_path = path.dirname(path.realpath(__file__))

@unique
class ErrorType(Enum):
    NONE = 0
    TOTALVIEW = 1
    LEFTVIEW = 2
    VIEWDIST = 3


class Generator():
    def __init__(self):
        self.workdir = dir_path + '/binary/'
        self.textureFile=''
        self.depthFile=''
        self.cameraFile=self.workdir+'cfg/cam.cfg'
        self.saveDir=''

        self.totalViews = 28
        self.leftViews = 14
        self.rightViews = 14
        self.viewDistant = 0.07

        self.width = 1920
        self.height = 1088
        self.frames = 50
        self.skips = 0

        self.FocalLen = 1732.875727
        self.CShift = 943.231169
        self.ZNear = -34.506386
        self.ZFar = -2760.510889

    def check(self):
        if self.totalViews < 2:
            return ErrorType.TOTALVIEW
        elif self.leftViews > self.totalViews:
            return ErrorType.LEFTVIEW
        elif (self.leftViews - 1) * self.viewDistant > 1 or (self.rightViews - 1) * self.viewDistant > 1:
            return ErrorType.VIEWDIST
        else:
            return ErrorType.NONE

    def add_input(self, confinput):
        with open(confinput, 'a') as file:
            file.writelines('VideoInputFile_0 : ' + self.textureFile +
                            '\nDepthInputFile_0: ' + self.depthFile + '\n')
            file.writelines('VideoInputFile_1 : ' + self.textureFile +
                            '\nDepthInputFile_1: ' + self.depthFile + '\n')
        file.close()

    def del_input(self, confinput):
        f = open(confinput, 'r')
        lines = f.readlines()
        f.close()
        f = open(confinput, 'w')
        for line in lines:
            if 'VideoInputFile_0 ' in line or 'DepthInputFile_0' in line or 'VideoInputFile_1 ' in line or 'DepthInputFile_1' in line:
                continue
            else:
                f.write(line)
        f.close()


    def run(self):
        self.cameraFile = self.workdir + 'cfg/cam.cfg'
        conffile = self.workdir + 'cfg/left_render.cfg'
        self.add_input(conffile)
        confinput = ' -c ' + conffile

        caminput  = ' -cpf ' + self.cameraFile

        inputwidth = ' -wdt ' + str(self.width)
        inputheight = ' -hgt ' + str(self.height)
        inputframes =  ' -f ' + str(self.frames)
        inputskips = ' -fs ' + str(self.skips)
        system(self.workdir +'bin/TAppRendererStatic'+ confinput
               + caminput + inputwidth + inputheight + inputframes + inputskips + ' && cd ..')
        self.del_input(conffile)

if __name__ == '__main__':
    gen = Generator()
    gen.textureFile = '/home/lixiang/S02_PoznanStreet/S02_PoznanStreet/PoznanStreet_1920x1088_25_05.yuv'
    gen.depthFile = '/home/lixiang/S02_PoznanStreet/S02_PoznanStreet/depth_PoznanStreet_1920x1088_25_05.yuv'
    gen.run()



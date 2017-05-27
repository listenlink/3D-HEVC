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

        self.cameraFile = self.workdir + 'cfg/cam.cfg'

    def check(self):
        if self.totalViews < 1:
            return ErrorType.TOTALVIEW
        elif self.leftViews > self.totalViews:
            return ErrorType.LEFTVIEW
        elif (self.leftViews - 1) * self.viewDistant > 1 or (self.rightViews - 1) * self.viewDistant > 1:
            return ErrorType.VIEWDIST
        else:
            return ErrorType.NONE

    def add_config(self, confinput, isRight):
        syncview, syncdir = self.get_sync_view(isRight)
        with open(confinput, 'a') as file:
            file.writelines('VideoInputFile_0 : ' + self.textureFile +
                            '\nDepthInputFile_0: ' + self.depthFile + '\n')
            file.writelines('VideoInputFile_1 : ' + self.textureFile +
                            '\nDepthInputFile_1: ' + self.depthFile + '\n')
            file.writelines('SynthViewCameraNumbers : ' + syncview + '\n')
            file.writelines('SynthOutputFileBaseName : ' + syncdir + '\n')
        file.close()

    def get_sync_view(self, isRight):
        syncview =''
        syncdir = self.saveDir
        if isRight:
            syncview = str(self.viewDistant) + ' : ' + str(self.viewDistant) + ' : ' + str(self.viewDistant * (self.rightViews + 0.5))
            syncdir = syncdir + '/synth_right$.yuv'
        else:
            syncview = '1 : ' + str(0 - self.viewDistant) + ' : ' + str(1 - self.viewDistant * self.leftViews)
            syncdir = syncdir + '/synth_left$.yuv'

        return syncview, syncdir

    def clean_config(self, confinput):
        f = open(confinput, 'r')
        lines = f.readlines()
        f.close()
        f = open(confinput, 'w')
        match_list = ['VideoInputFile_0', 'DepthInputFile_0',
                      'VideoInputFile_1', 'DepthInputFile_1',
                      'SynthViewCameraNumbers', 'SynthOutputFileBaseName']
        #if any(x in str for x in a):
        for line in lines:
            if any(x in line for x in match_list):
                continue
            else:
                f.write(line)
        f.close()

    def gen(self, render_config_name, command, isRight = True):
        config_file = self.workdir + render_config_name
        self.add_config(config_file, isRight)
        config_command = ' -c ' + config_file

        system(self.workdir +'bin/TAppRendererStatic'+ config_command
               + command + ' && cd ..')
        self.clean_config(config_file)

    def run(self):
        render_dir = 'cfg/'

        cam_parms = ' -cpf ' + self.cameraFile
        width_parms = ' -wdt ' + str(self.width)
        height_parms = ' -hgt ' + str(self.height)
        frames_parms =  ' -f ' + str(self.frames)
        skips_parms = ' -fs ' + str(self.skips)
        command_parms = cam_parms + width_parms + height_parms + frames_parms + skips_parms
        if self.rightViews > 0:
            self.gen(render_dir + 'right_render.cfg', command_parms, True)
        if self.leftViews > 0:
            self.gen(render_dir + 'left_render.cfg', command_parms, False)

def test():
    gen = Generator()
    gen.totalViews = gen.leftViews = 2
    gen.rightViews = 0
    gen.saveDir = '/home/lixiang/Documents/'
    gen.textureFile = '/home/lixiang/S02_PoznanStreet/S02_PoznanStreet/PoznanStreet_1920x1088_25_05.yuv'
    gen.depthFile = '/home/lixiang/S02_PoznanStreet/S02_PoznanStreet/depth_PoznanStreet_1920x1088_25_05.yuv'
    gen.run()

if __name__ == '__main__':
    test()




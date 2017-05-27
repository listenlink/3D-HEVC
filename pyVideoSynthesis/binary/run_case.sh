#!/bin/sh
# Please modify the paramters in cfg/Renderer/rendering_s2/rendering_2view_orgData.cfg
# VideoInputFile_0 : /home/linlixia/TestSequences/PoznanStreet_1920x1088_25_05.yuv
# DepthInputFile_0 : /home/linlixia/TestSequences/depth_PoznanStreet_1920x1088_25_05.yuv
# to be the correct folder on your system
export DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#DIR=$pwd
$DIR/bin/TAppRendererStatic -c $DIR/cfg/Renderer/rendering_s2/rendering_2view_orgData.cfg

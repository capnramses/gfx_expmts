#!/bin/bash
g++ main.c -I /usr/include/opencv4/ \
-I ../common/include/stb/ \
-lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_videoio -lopencv_imgcodecs -lopencv_flann
#libs listed on https://docs.opencv.org/master/d7/d16/tutorial_linux_eclipse.html

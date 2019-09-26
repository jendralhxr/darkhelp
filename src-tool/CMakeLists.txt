# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id: CMakeLists.txt 2813 2019-08-26 01:37:14Z stephane $


ADD_EXECUTABLE ( DarkHelp main.cpp)
TARGET_LINK_LIBRARIES ( DarkHelp Threads::Threads darkhelp ${Darknet} ${OpenCV_LIBS} )
INSTALL ( TARGETS DarkHelp DESTINATION bin )

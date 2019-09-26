# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id: CM_dependencies.cmake 2805 2019-08-23 08:28:19Z stephane $


FIND_PACKAGE ( Threads REQUIRED )
FIND_PACKAGE ( OpenCV REQUIRED )
FIND_LIBRARY ( Darknet darknet )

INCLUDE_DIRECTORIES ( ${OpenCV_INCLUDE_DIRS} )

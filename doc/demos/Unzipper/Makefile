###################################################
# Project definition
#
PROJECT = Zipper
TARGET = demo_unzip
DESCRIPTION = Demo for unzipping a zip
STANDARD = --std=c++11
BUILD_TYPE = release

###################################################
# Location of the project directory and Makefiles
#
P := ../../..
M := $(P)/.makefile
include $(M)/Makefile.header

###################################################
# Inform Makefile where to find *.cpp and *.o files
#
VPATH +=

###################################################
# Inform Makefile where to find header files
#
INCLUDES +=

###################################################
# Compilation
#
CXXFLAGS +=

###################################################
# Project defines.
#
DEFINES +=

###################################################
# Compiled files
#

OBJS += demo_unzip.o

###################################################
# Libraries.
#
PKG_LIBS += zipper

###################################################
# Compile static and shared libraries
all: $(TARGET)

###################################################
# Sharable informations between all Makefiles
include $(M)/Makefile.footer

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
VPATH += $(P)/src $(P)/src/utils $(THIRDPART)

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I. -I$(P)/include -I$(P)/src

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
THIRDPART_LIBS += $(abspath $(P)/$(BUILD)/libzipper.a)
#PKG_LIBS += zipper

###################################################
# Compile static and shared libraries
all: $(TARGET)

###################################################
# Sharable informations between all Makefiles
include $(M)/Makefile.footer

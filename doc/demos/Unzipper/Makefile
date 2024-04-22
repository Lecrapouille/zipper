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
VPATH += $(P) src

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I$(P)/include -Isrc

###################################################
# Compilation
#
CXXFLAGS +=

###################################################
# Project defines.
#
DEFINES +=

###################################################
# Compile the demo as standalone application.
#
OBJS += demo_unzip.o

###################################################
# Libraries: link against our newly compiled lib
#
THIRDPART_LIBS += $(abspath $(P)/$(BUILD)/libzipper.a)

###################################################
# Compile static and shared libraries
all: $(TARGET)

###################################################
# Sharable informations between all Makefiles
include $(M)/Makefile.footer

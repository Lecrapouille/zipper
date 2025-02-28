###################################################
# Location of the project directory and Makefiles
#
P := ../../..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
CXX_STANDARD := --std=c++17
TARGET_NAME := zipper-demo
TARGET_DESCRIPTION := A demo for zipping using $(PROJECT_NAME)
include $(M)/project/Makefile

###################################################
# Compile the standalone application
#
SRC_FILES := $(call rwildcard,src,*.cpp)
INCLUDES := $(P)/include src
VPATH := $(P)/demo
INTERNAL_LIBS := $(call internal-lib,$(PROJECT_NAME))

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile

$(INTERNAL_LIBS):
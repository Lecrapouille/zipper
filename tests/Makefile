###################################################
# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)-tests
TARGET_DESCRIPTION := Unit tests for $(PROJECT_NAME)
COMPILATION_MODE := test
include $(M)/project/Makefile

###################################################
# Compile the unit tests
#
SRC_FILES := $(call rwildcard,$(P)/src,*.cpp)
SRC_FILES += $(call rwildcard,$(P)/tests,*.cpp)
INCLUDES := $(P) $(P)/include $(P)/src
VPATH := $(P) $(P)/src $(P)/src/utils
DEFINES += -DPWD=\"$(shell pwd)\"
DEFINES += -DHAVE_AES
ifeq ($(ARCHI),Windows)
    LIB_FILES += $(P)/src/utils/dirent.c
    DEFINES += -DUSE_WINDOWS
else
    DEFINES += -UUSE_WINDOWS
endif
THIRDPART_LIBS := $(abspath $(THIRDPART_DIR)/minizip/build/libminizip.a)
THIRDPART_LIBS += $(abspath $(THIRDPART_DIR)/minizip/build/libaes.a)
THIRDPART_LIBS += $(abspath $(THIRDPART_DIR)/zlib-ng/build/libz.a)
PKG_LIBS += gtest gmock

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile
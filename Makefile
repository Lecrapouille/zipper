###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := Zipper is a C++ library for creating and reading zip archives.
CXX_STANDARD := --std=c++14
include $(M)/project/Makefile

###################################################
# Compile shared and static libraries
#
LIB_FILES := $(call rwildcard,src,*.cpp)
ifeq ($(OS),Windows)
    LIB_FILES += src/utils/dirent.c
endif
INCLUDES := $(P) $(P)/include $(P)/src $(THIRD_PARTIES_DIR)
VPATH := $(P)/src $(P)/src/utils $(THIRD_PARTIES_DIR)
DEFINES += -DHAVE_AES

###################################################
# Third party libraries (compiled with make compile-external-libs)
#
THIRD_PARTIES_LIBS := $(abspath $(THIRD_PARTIES_DIR)/minizip/build/libminizip.a)
THIRD_PARTIES_LIBS += $(abspath $(THIRD_PARTIES_DIR)/minizip/build/libaes.a)
THIRD_PARTIES_LIBS += $(abspath $(THIRD_PARTIES_DIR)/zlib-ng/build/libz.a)

###################################################
# Documentation
#
PATH_PROJECT_LOGO := $(PROJECT_DOC_DIR)/doxygen-logo.png

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile

###################################################
# Extra rules
#
all:: demos

.PHONY: demos
demos: $(TARGET_STATIC_LIB_NAME)
	$(Q)$(MAKE) --no-print-directory --directory=doc/demos/Unzipper all
	$(Q)$(MAKE) --no-print-directory --directory=doc/demos/Zipper all

clean::
	$(Q)$(MAKE) --no-print-directory --directory=doc/demos/Unzipper clean
	$(Q)$(MAKE) --no-print-directory --directory=doc/demos/Zipper clean

install::
	$(Q)$(MAKE) --no-print-directory --directory=doc/demos/Unzipper install
	$(Q)$(MAKE) --no-print-directory --directory=doc/demos/Zipper install

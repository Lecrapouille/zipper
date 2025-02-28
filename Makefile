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
TARGET_DESCRIPTION := An open source implementation of the SimCity 2013 simulation engine GlassBox
include $(M)/project/Makefile

###################################################
# Compile shared and static libraries
#
LIB_FILES := $(call rwildcard,src,*.cpp)
INCLUDES := $(P)/include $(P)/src $(P)
VPATH := $(P)/src $(P)/src/utils $(THIRDPART_DIR)
ifeq ($(OS),Windows)
    LIB_FILES += src/utils/dirent.c
    DEFINES += -DUSE_WINDOWS -DHAVE_AES
else
    DEFINES += -UUSE_WINDOWS -DHAVE_AES
endif
THIRDPART_LIBS := $(abspath $(THIRDPART_DIR)/minizip/build/libminizip.a)
THIRDPART_LIBS += $(abspath $(THIRDPART_DIR)/minizip/build/libaes.a)
THIRDPART_LIBS += $(abspath $(THIRDPART_DIR)/zlib-ng/build/libz.a)

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

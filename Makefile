###################################################
# Project definition
#
PROJECT = Zipper
TARGET = $(PROJECT)
DESCRIPTION = C++ wrapper around minizip compression library
STANDARD = --std=c++11
BUILD_TYPE = release

###################################################
# Documentation
#
LOGO = logo.png

###################################################
# Location of the project directory and Makefiles
#
P := .
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
#CXXFLAGS += -Wno-undef

###################################################
# Project defines.
#
DEFINES += -DHAVE_AES
ifeq ($(ARCHI),Windows)
DEFINES += -DUSE_WINDOWS
else
DEFINES += -UUSE_WINDOWS
endif

###################################################
# Compiled files
#
ifeq ($(ARCHI),Windows)
LIB_OBJS += dirent.o
endif
LIB_OBJS += Timestamp.o Path.o Zipper.o Unzipper.o

###################################################
# Libraries.
#
PKG_LIBS +=
LINKER_FLAGS +=
THIRDPART_LIBS += \
    $(abspath $(THIRDPART)/minizip/build/libminizip.a) \
    $(abspath $(THIRDPART)/minizip/build/libaes.a) \
    $(abspath $(THIRDPART)/zlib-ng/build/libz.a)

###################################################
# Compile static and shared libraries
all: $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE)

###################################################
# Compile the demo as standalone application.
.PHONY: demos
demos: | $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET)
	@$(call print-from,"Compiling demos",$(PROJECT),unzip)
	$(MAKE) -C doc/demos/Unzipper all

###################################################
# Compile and launch unit tests and generate the code coverage html report.
.PHONY: unit-tests
.PHONY: check
unit-tests check:
	@$(call print-simple,"Compiling unit tests")
	@$(MAKE) -C tests coverage

###################################################
# Install project. You need to be root.
.PHONY: install
install: $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE)
	@$(call INSTALL_DOCUMENTATION)
	@$(call INSTALL_PROJECT_LIBRARIES)
	@$(call INSTALL_PROJECT_HEADERS)

###################################################
# Clean the whole project.
.PHONY: veryclean
veryclean: clean
	@rm -fr cov-int $(PROJECT).tgz *.log foo 2> /dev/null
	@(cd tests && $(MAKE) -s clean)
	@$(call print-simple,"Cleaning","$(THIRDPART)")
	@rm -fr $(THIRDPART)/*/ doc/html 2> /dev/null

###################################################
# Sharable informations between all Makefiles
include $(M)/Makefile.footer

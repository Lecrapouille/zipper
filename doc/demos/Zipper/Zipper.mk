###################################################
# A demo for zipping. Paths are relative to this directory.
###################################################

TARGETS += zipper-demo
zipper-demo.DESCRIPTION := A demo for zipping using zipper
zipper-demo.SOURCES := src/**/*.cpp
zipper-demo.INCLUDES := src
zipper-demo.DEPENDS := zipper

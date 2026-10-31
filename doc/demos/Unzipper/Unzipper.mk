###################################################
# A demo for unzipping. Paths are relative to this directory.
###################################################

TARGETS += unzipper-demo
unzipper-demo.DESCRIPTION := A demo for unzipping using zipper
unzipper-demo.SOURCES := src/**/*.cpp
unzipper-demo.INCLUDES := src
unzipper-demo.DEPENDS := zipper

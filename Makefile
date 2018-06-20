RACK_DIR ?= ../..
SLUG = Befaco
VERSION = 0.6.0

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res

BINARIES += src/SpringReverbIR.pcm

include $(RACK_DIR)/plugin.mk

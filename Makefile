RACK_DIR ?= ../..

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/noise-plethora/*/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res

BINARIES += src/SpringReverbIR.pcm

include $(RACK_DIR)/plugin.mk

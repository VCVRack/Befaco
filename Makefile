RACK_DIR ?= ../..

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res

BINARIES += src/SpringReverbIR.pcm

include $(RACK_DIR)/plugin.mk

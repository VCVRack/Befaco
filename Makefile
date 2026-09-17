RACK_DIR ?= ../..

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/random8_core/*.cpp)
SOURCES += $(wildcard src/noise-plethora/*/*.cpp)

# OwlProgram sources (mirrors Befaco Oneiroi integration)
SOURCES += $(wildcard libs/OwlProgram/LibSource/*.cpp)
SOURCES += $(wildcard libs/OwlProgram/Libraries/KissFFT/*.c)
SOURCES += $(wildcard libs/OwlProgram/LibSource/*.c)

SOURCES := $(filter-out libs/OwlProgram/LibSource/ColourScreenPatch.cpp,$(SOURCES))
SOURCES := $(filter-out libs/OwlProgram/LibSource/MonochromeScreenPatch.cpp,$(SOURCES))
SOURCES := $(filter-out libs/OwlProgram/LibSource/PatchParameter.cpp,$(SOURCES))

FLAGS += -DVCV -Ilibs/OwlProgram/LibSource -Ilibs/OwlProgram/Source -Ilibs/OwlProgram/Libraries/KissFFT -Ilibs/OwlProgram/Libraries -Ilibs/Iroi

DISTRIBUTABLES += $(wildcard LICENSE*) res

include $(RACK_DIR)/plugin.mk

CXXFLAGS += -std=c++17

# debug only
# CXXFLAGS += -g -O0 

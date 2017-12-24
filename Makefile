SLUG = Befaco
VERSION = 0.5.0

FLAGS = -I./pffft -DPFFFT_SIMD_DISABLE

SOURCES += $(wildcard src/*.cpp) pffft/pffft.c

DISTRIBUTABLES += $(wildcard LICENSE*) res


include ../../plugin.mk


FLAGS = -I./pffft -DPFFFT_SIMD_DISABLE

SOURCES = $(wildcard src/*.cpp) pffft/pffft.c

include ../../plugin.mk

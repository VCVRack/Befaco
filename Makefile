
FLAGS = -I./pffft -DPFFFT_SIMD_DISABLE

SOURCES = $(wildcard src/*.cpp) pffft/pffft.c

include ../../plugin.mk


dist: all
	mkdir -p dist/Befaco
	cp LICENSE* dist/Befaco/
	cp plugin.* dist/Befaco/
	cp -R res dist/Befaco/
	cd dist && zip -5 -r Befaco-$(VERSION)-$(ARCH).zip Befaco

BUILD_DIR=build
MOSAIC_DIR=$(HOME)/mosaic

SOURCES=$(wildcard *.mos)
OBJECTS=$(patsubst %.mos,$(BUILD_DIR)/%.exe,$(SOURCES) )

all: $(OBJECTS)

clean:
	rm -f $(BUILD_DIR)/*.exe

build/%.exe: %.mos $(MOSAIC_DIR)/build/mosaicc
	$(MOSAIC_DIR)/mcc $<


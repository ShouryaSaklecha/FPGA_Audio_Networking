CC       ?= gcc
CXX      ?= g++
LANGFLAG = -x c++
CPPFLAGS += -I include/
CFLAGS   += -g -Wall -O2 -std=c++11
LDFLAGS  += $(LIBS) -lpthread -lz -rdynamic
BUILD_DIR = build

BINARY = sample256
OBJ = $(BUILD_DIR)/main.o \
      $(BUILD_DIR)/audio_i2s.o \
      $(BUILD_DIR)/axi_dma.o \

.PHONY: clean

all: $(BINARY) ctrl_bus_test

ctrl_bus_test: src/ctrl_bus.c
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^

$(BINARY): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/main.o: src/main.c include/audio_i2s.h
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

$(BUILD_DIR)/audio_i2s.o: src/audio_i2s.c include/audio_i2s.h include/axi_dma.h include/misc.h
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

$(BUILD_DIR)/axi_dma.o: src/axi_dma.c include/axi_dma.h include/misc.h
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LANGFLAG) $< -c -o $@

clean:
	rm -rf $(BINARY) $(BUILD_DIR)/*.o ctrl_bus

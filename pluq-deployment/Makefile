# Makefile for PluQ test tools

# Detect OS
UNAME_S := $(shell uname -s)

# Set compiler
CC = gcc

# Common flags
CFLAGS = -Wall -I../Quake

# Platform-specific library paths
ifeq ($(UNAME_S),Linux)
    PLUQ_LIB_DIR = ../Linux/pluq
    LDFLAGS_PLATFORM = -lpthread
endif
ifeq ($(UNAME_S),Darwin)
    PLUQ_LIB_DIR = ../macOS/pluq
    LDFLAGS_PLATFORM = -lpthread
endif
ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    PLUQ_LIB_DIR = ../Windows/pluq/lib/x64
    PLUQ_INC_DIR = ../Windows/pluq/include
    LDFLAGS_PLATFORM = -lws2_32
endif
ifeq ($(findstring MSYS,$(UNAME_S)),MSYS)
    PLUQ_LIB_DIR = ../Windows/pluq/lib/x64
    PLUQ_INC_DIR = ../Windows/pluq/include
    LDFLAGS_PLATFORM = -lws2_32
endif

# Default include and lib dirs for Unix
ifndef PLUQ_INC_DIR
    PLUQ_INC_DIR = $(PLUQ_LIB_DIR)/include
endif

# Add prebuild include and lib paths
CFLAGS += -I$(PLUQ_INC_DIR)
LDFLAGS = -L$(PLUQ_LIB_DIR)/lib -lnng $(LDFLAGS_PLATFORM)
LDFLAGS_FLATCC = -L$(PLUQ_LIB_DIR)/lib -lnng -lflatccrt $(LDFLAGS_PLATFORM)

all: test-monitor test-command test-input-receiver test-backend-simulator

test-monitor: test-monitor.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@
	@echo "Built test-monitor"

test-command: test-command.c
	$(CC) $(CFLAGS) $< $(LDFLAGS_FLATCC) -o $@
	@echo "Built test-command"

test-input-receiver: test-input-receiver.c
	$(CC) $(CFLAGS) $< $(LDFLAGS_FLATCC) -o $@
	@echo "Built test-input-receiver"

test-backend-simulator: test-backend-simulator.c
	$(CC) $(CFLAGS) $< $(LDFLAGS_FLATCC) -o $@
	@echo "Built test-backend-simulator"

clean:
	rm -f test-monitor test-command test-input-receiver test-backend-simulator

.PHONY: all clean

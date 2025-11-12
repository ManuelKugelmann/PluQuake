# Makefile for PluQ test tools

CC = gcc
CFLAGS = -Wall -I../Quake -I../Quake/dependencies/include
LDFLAGS = -L../Quake/dependencies/lib -lnng -lpthread
LDFLAGS_FLATCC = -L../Quake/dependencies/lib -lnng -lflatccrt -lpthread

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

.PHONY: all clean

all: ask2-fork ask2-dfs_signals ask4 ask2-signals

CC = gcc
CFLAGS = -g -Wall -O2
SHELL= /bin/bash

ask2-fork: ask2-fork.c proc-common.c
	$(CC) $(CFLAGS) $^ -o $@

ask2-dfs_signals: ask2-dfs_signals.c proc-common.c tree.c
	$(CC) $(CFLAGS) $^ -o $@

ask2-signals: ask2-signals.c proc-common.c tree.c
	$(CC) $(CFLAGS) $^ -o $@
        
ask4 : ask4.c proc-common.c tree.c
	$(CC) $(CFLAGS) $^ -o $@

%.s: %.c
	$(CC) $(CFLAGS) -S -fverbose-asm $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.i: %.c
	gcc -Wall -E $< | indent -kr > $@

clean: 
	rm -f *.o tree-example fork-example pstree-this ask2-{fork,tree,signals,pipes}

NAME := $(shell basename $(PWD))
export MODULE := M6
all: $(NAME)-64.so $(NAME)-32.so
CFLAGS += -U_FORTIFY_SOURCE

include ../Makefile
.PHONY: vim
vim:
	@vim kvdb.c


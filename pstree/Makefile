NAME := $(shell basename $(PWD))
export MODULE := M1
all: $(NAME)-64 $(NAME)-32

include ../Makefile

.PHONY: vim run com
vim:
	@vim pstree.c
run: all git
	@echo "The result on 64-bit machines"
	@./pstree-64
	@echo "The result on 32-bit machines"
	@./pstree-32
com:
	@./pstree-32 | wc -l
	@./pstree-32 --show-pids --numeric-sort | wc -l
	@./pstree-32 --show-pids | wc -l
	@./pstree-32 --numeric-sort | wc -l

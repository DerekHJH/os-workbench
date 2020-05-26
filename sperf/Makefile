NAME := $(shell basename $(PWD))
export MODULE := M3
all: $(NAME)-64 $(NAME)-32

include ../Makefile

.PHONY: vim run com
vim:
	@vim $(NAME).c
run: all git
	@echo "The result on 64-bit machines"
	@./$(NAME)-64 ls

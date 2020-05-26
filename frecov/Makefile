NAME := $(shell basename $(PWD))
export MODULE := M5
all: $(NAME)-64 $(NAME)-32
LDFLAGS += -lm

include ../Makefile

.PHONY: run vim
vim:
	@vim $(NAME).c
run: all git
	@echo "The result on 64-bit machines"
	@./$(NAME)-64 ../../M5-frecov.img


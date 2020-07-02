NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c" -o -name "*.S")
INC_DIR        := include/ framework/
export MODULE  := L3
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH := x86_64-qemu
endif

include $(AM_HOME)/Makefile.app
include ../Makefile.lab
image: git

test:
	@make 
	@tools/mkfs 64 build/kernel-x86_64-qemu tools/fs-img/
	@qemu-system-x86_64 -serial stdio -machine accel=tcg -smp 1 -drive format=raw,file=build/kernel-x86_64-qemu

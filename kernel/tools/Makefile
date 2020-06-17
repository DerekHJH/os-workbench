.PHONY: clean run vim

mkfs: mkfs.c
	@gcc -O1 -Wall -Werror -o $@ -I../framework $<

clean:
	rm -rf mkfs

run: mkfs
	@./mkfs 64 haha temp/

vim:
	@vim mkfs.c

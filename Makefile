CFLAGS = -Wall -Wextra -Wconversion -O0 -ggdb

.PHONY: all clean

all: elf_loader.out test.out

elf_loader.out: main.c elf_loader.c
	$(CC) $(CFLAGS) $+ -o $@

test.out: test.c
	$(CC) $(CFLAGS) -nostartfiles -fPIC -static $+ -o $@

clean:
	rm -f *.out

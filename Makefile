CFLAGS = -Wall -Wextra -Wconversion -O0 -ggdb
PROD = main

.PHONY: clean

all: $(PROD) test.out

$(PROD): main.c elf_loader.c
	$(CC) $(CFLAGS) $+ -o $@

test.out: test.c
	$(CC) $(CFLAGS) -nostartfiles -fPIE -pie -static $+ -o $@

clean:
	rm -f $(PROD) *.out *.o

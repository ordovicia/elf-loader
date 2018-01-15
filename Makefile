CFLAGS = -Wall -Wextra -Wconversion -O0 -ggdb
PROD = main

.PHONY: clean

$(PROD): main.tmp elf_loader.c
	$(CC) $(CFLAGS) $+ -o $@
	rm $<

main.tmp: main.o test.out
	objcopy --add-section=elf_exec=test.out \
		--set-section-flags=elf_exec=contents,alloc,load,code $< $@

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

test.out: test.c
	$(CC) $(CFLAGS) -nostartfiles -fPIE -pie -static $+ -o $@

clean:
	rm -f $(PROD) *.out *.o

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "elf_loader.h"

#define LIBC_FAIL(msg)      \
    do {                    \
        perror(#msg);       \
        exit(EXIT_FAILURE); \
    } while (0);

#define FAIL(msg)           \
    do {                    \
        fputs(msg, stderr); \
        exit(EXIT_FAILURE); \
    } while (0);

extern char __start_elf_exec;
extern char __stop_elf_exec;

int main(void)
{
    const void* elf_ptr = &__start_elf_exec;

    Elf64Exec elf64_exec;
    if (parse_elf64(elf_ptr, &elf64_exec) == -1)
        FAIL("Failed to parse file\n");

    if (load_elf64(&elf64_exec) == -1)
        FAIL("Failed to load ELF\n");

    return 0;
}

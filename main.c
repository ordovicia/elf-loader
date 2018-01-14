#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

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

const void* get_mmap_ptr(const char* file_name);

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    const void* elf_ptr = get_mmap_ptr(argv[1]);

    Elf64Exec elf64_exec;
    if (parse_elf64(elf_ptr, &elf64_exec) == -1)
        FAIL("Failed to parse file\n");

    if (load_elf64(&elf64_exec) == -1)
        FAIL("Failed to load ELF\n");

    return 0;
}

const void* get_mmap_ptr(const char* file_name)
{
    int fd = open(file_name, O_RDONLY);
    if (fd == -1)
        LIBC_FAIL(open);

    struct stat sb;
    if (fstat(fd, &sb) == -1)
        LIBC_FAIL(fstat);
    size_t file_size = (size_t)sb.st_size;

    const void* elf_ptr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (elf_ptr == MAP_FAILED)
        LIBC_FAIL(mmap);

    return elf_ptr;
}

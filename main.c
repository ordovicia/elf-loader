#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "elf_loader.h"

#define FAIL(msg)           \
    do {                    \
        fputs(msg, stderr); \
        exit(EXIT_FAILURE); \
    } while (0);

#define FAIL_LIBC(msg)      \
    do {                    \
        perror(#msg);       \
        exit(EXIT_FAILURE); \
    } while (0);

typedef struct {
    void* ptr;
    size_t size;
} MMap;

MMap mmap_path(const char* path);

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return 1;
    }

    MMap elf_mmap = mmap_path(argv[1]);

    Elf64Exec elf64_exec;
    if (elf64_parse(elf_mmap.ptr, &elf64_exec) == -1) {
        FAIL("Failed to parse file\n");
    }

    int retval;
    if (elf64_load(&elf64_exec, &retval) == -1) {
        FAIL("Failed to load ELF\n");
    }

    puts("");
    printf("Execution succeeded. Return value: %d\n", retval);

    if (munmap(elf_mmap.ptr, elf_mmap.size) == -1) {
        FAIL_LIBC(munmap);
    }

    return 0;
}

MMap mmap_path(const char* path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        FAIL_LIBC(open);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        FAIL_LIBC(fstat);
    }

    MMap result;
    result.size = (size_t)sb.st_size;

    result.ptr = mmap(NULL, result.size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (result.ptr == MAP_FAILED) {
        FAIL_LIBC(mmap);
    }

    return result;
}

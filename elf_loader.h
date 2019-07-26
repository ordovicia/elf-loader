#pragma once

#include "elfhead.h"

typedef struct {
    const uint8_t* header;
    const Elf64_Ehdr* elf64_header;
    Elf64_Xword mem_size;
    Elf64_Xword align;
} Elf64Exec;

// Parses mmap'ed ELF file into Elf64Exec and prints the header information.
// Returns 0 on success, -1 on failure.
int elf64_parse(const void* elf_ptr, Elf64Exec* elf64_exec);

// Loads and executes given ELF executable.
// Stores the return value of the executable in retval.
// Returns 0 on success, -1 on failure.
int elf64_load(const Elf64Exec* elf64_exec, int* retval);

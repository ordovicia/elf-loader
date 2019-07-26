#include <stdio.h>
#include <stdlib.h>    // posix_memalign
#include <string.h>    // memcpy & memset
#include <sys/mman.h>  // mprotect

#include "elf_loader.h"

static inline int is_elf(const Elf64_Ehdr* e_hdr);
static inline int is_elf64(const Elf64_Ehdr* e_hdr);

static inline int is_abi_sysv(const Elf64_Ehdr* e_hdr);
static inline int is_abi_gnu(const Elf64_Ehdr* e_hdr);

int elf64_parse(const void* const elf_ptr, Elf64Exec* elf64_exec)
{
    const uint8_t* hdr = (const uint8_t*)elf_ptr;
    const Elf64_Ehdr* e_hdr = (const Elf64_Ehdr*)elf_ptr;

    // Validate ELF header
    if (!is_elf(e_hdr) || !is_elf64(e_hdr)
        || (!is_abi_sysv(e_hdr) && !is_abi_gnu(e_hdr))) {
        fputs("Invalid ELF header\n", stderr);
        return -1;
    }

    puts("      Class: ELF64");
    printf("        ABI: %d\n", e_hdr->e_ident[EI_OSABI]);
    printf("Entry point: 0x%08lx\n", e_hdr->e_entry);

    if (e_hdr->e_type != ET_EXEC) {
        fputs("Not executable ELF file\n", stderr);
        return -1;
    }

    // Sections
    Elf64_Xword total_mem_size = 0;

    fputs("   Sections: ", stdout);
    if (e_hdr->e_shstrndx != SHN_UNDEF) {
        const Elf64_Shdr* sec_hdr_tbl
            = (const Elf64_Shdr*)(hdr + e_hdr->e_shoff);
        const Elf64_Shdr* str_tbl_sec = &sec_hdr_tbl[e_hdr->e_shstrndx];
        const char* sec_hdr_str_tbl
            = (const char*)(hdr + str_tbl_sec->sh_offset);

        for (int i = 0; i < e_hdr->e_shnum; i++) {
            const Elf64_Shdr* sct_hdr
                = (const Elf64_Shdr*)(hdr + e_hdr->e_shoff
                                      + e_hdr->e_shentsize * i);
            const char* sec_name = sec_hdr_str_tbl + sct_hdr->sh_name;
            if (i > 0) {
                fputs("             ", stdout);
            }
            printf("[%2d] %-24s  size: 0x%08lx  offset: 0x%08lx\n",
                i, sec_name, sct_hdr->sh_size, sct_hdr->sh_offset);
            Elf64_Xword mem_size = sct_hdr->sh_addr + sct_hdr->sh_offset;
            if (total_mem_size < mem_size) {
                total_mem_size = mem_size;
            }
        }
    } else {
        puts(" None");
    }

    printf("    Memsize: 0x%08lx bytes\n", total_mem_size);

    // Segments
    Elf64_Xword mem_size = 0;
    Elf64_Xword align = sizeof(void*);

    fputs("   Segments: ", stdout);
    for (int i = 0; i < e_hdr->e_phnum; i++) {
        const Elf64_Phdr* prog_hdr
            = (const Elf64_Phdr*)(hdr + e_hdr->e_phoff
                                  + e_hdr->e_phentsize * i);

        Elf64_Xword ms = prog_hdr->p_vaddr + prog_hdr->p_memsz;
        if (mem_size < ms) {
            mem_size = ms;
        }
        if (align < prog_hdr->p_align) {
            align = prog_hdr->p_align;
        }

        switch (prog_hdr->p_type) {
        case PT_LOAD:
            if (i > 0) {
                fputs("             ", stdout);
            }
            printf("[%2d] vaddr: 0x%08lx  size: 0x%08lx  align: 0x%08lx\n",
                i, prog_hdr->p_vaddr, prog_hdr->p_memsz, prog_hdr->p_align);
            break;
        default:
            break;
        }
    }

    printf("    Memsize: 0x%08lx bytes\n", mem_size);
    printf("      Align: 0x%08lx\n", align);

    elf64_exec->header = hdr;
    elf64_exec->elf64_header = e_hdr;
    elf64_exec->mem_size = mem_size;
    elf64_exec->align = align;

    return 0;
}

int elf64_load(const Elf64Exec* elf64_exec, int* retval)
{
    const uint8_t* hdr = elf64_exec->header;
    const Elf64_Ehdr* e_hdr = elf64_exec->elf64_header;

    void* mem_buf;
    if (posix_memalign(&mem_buf, elf64_exec->align, elf64_exec->mem_size)) {
        perror("posix_memalign");
        return -1;
    }

    if (mprotect(
            mem_buf, elf64_exec->mem_size, PROT_READ | PROT_WRITE | PROT_EXEC)
        == -1) {
        perror("mprotect");
        return -1;
    }

    for (int i = 0; i < e_hdr->e_phnum; i++) {
        const Elf64_Phdr* prog_hdr
            = (const Elf64_Phdr*)(hdr + e_hdr->e_phoff
                                  + e_hdr->e_phentsize * i);

        // if (prog_hdr->p_memsz != 0) {
        //     virt_addr start = ptr2virtaddr(_membuffer) + prog_hdr->p_vaddr;
        //     virt_addr end = start + prog_hdr->p_memsz;
        //     _loader.MapAddr(start, end);
        // }

        switch (prog_hdr->p_type) {
        case PT_LOAD:
            memcpy(
                mem_buf + prog_hdr->p_vaddr, &hdr[prog_hdr->p_offset], prog_hdr->p_memsz);
            memset(mem_buf + prog_hdr->p_vaddr + prog_hdr->p_filesz,
                0,
                prog_hdr->p_memsz - prog_hdr->p_filesz);
            break;
        default:
            break;
        }
    }

    // Clear bss section
    for (int i = 0; i < e_hdr->e_shnum; i++) {
        const Elf64_Shdr* sct_hdr
            = (const Elf64_Shdr*)(hdr + e_hdr->e_shoff
                                  + e_hdr->e_shentsize * i);
        if (sct_hdr->sh_type == SHT_NOBITS) {
            if ((sct_hdr->sh_flags & SHF_ALLOC) != 0)
                memset(mem_buf + sct_hdr->sh_addr, 0, sct_hdr->sh_size);
        }
    }

    int (*entry_point)() = mem_buf + e_hdr->e_entry;
    *retval = entry_point();

    return 0;
}

#define EQ_ELF_IDENT(idx, val) \
    e_hdr->e_ident[(idx)] == (val)

static inline int is_elf(const Elf64_Ehdr* e_hdr)
{
    return EQ_ELF_IDENT(0, ELFMAG0)
           && EQ_ELF_IDENT(1, ELFMAG1)
           && EQ_ELF_IDENT(2, ELFMAG2)
           && EQ_ELF_IDENT(3, ELFMAG3);
}

static inline int is_elf64(const Elf64_Ehdr* e_hdr)
{
    return EQ_ELF_IDENT(EI_CLASS, ELFCLASS64);
}

static inline int is_abi_sysv(const Elf64_Ehdr* e_hdr)
{
    return EQ_ELF_IDENT(EI_OSABI, ELFOSABI_SYSV);
}

static inline int is_abi_gnu(const Elf64_Ehdr* e_hdr)
{
    return EQ_ELF_IDENT(EI_OSABI, ELFOSABI_GNU);
}

#include "../wumsloader/src/elfio/elf_types.hpp"
#include "utils/logger.h"
#include <coreinit/cache.h>

uint32_t load_loader_elf(unsigned char *baseAddress, char *elf_data, uint32_t fileSize) {
    ELFIO::Elf32_Ehdr *ehdr;
    ELFIO::Elf32_Phdr *phdrs;
    uint8_t *image;
    int32_t i;

    ehdr = (ELFIO::Elf32_Ehdr *) elf_data;

    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        return 0;
    }

    if (ehdr->e_phentsize != sizeof(ELFIO::Elf32_Phdr)) {
        return 0;
    }

    phdrs = (ELFIO::Elf32_Phdr *) (elf_data + ehdr->e_phoff);

    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD) {
            continue;
        }

        if (phdrs[i].p_filesz > phdrs[i].p_memsz) {
            continue;
        }

        if (!phdrs[i].p_filesz) {
            continue;
        }

        uint32_t p_paddr = phdrs[i].p_paddr + (uint32_t) baseAddress;
        image            = (uint8_t *) (elf_data + phdrs[i].p_offset);

        memcpy((void *) p_paddr, image, phdrs[i].p_filesz);
        DCFlushRange((void *) p_paddr, phdrs[i].p_filesz);

        if (phdrs[i].p_flags & PF_X) {
            ICInvalidateRange((void *) p_paddr, phdrs[i].p_memsz);
        }
    }

    //! clear BSS
    auto *shdr = (ELFIO::Elf32_Shdr *) (elf_data + ehdr->e_shoff);
    for (i = 0; i < ehdr->e_shnum; i++) {
        const char *section_name = ((const char *) elf_data) + shdr[ehdr->e_shstrndx].sh_offset + shdr[i].sh_name;
        if (section_name[0] == '.' && section_name[1] == 'b' && section_name[2] == 's' && section_name[3] == 's') {
            memset((void *) (shdr[i].sh_addr + baseAddress), 0, shdr[i].sh_size);
            DCFlushRange((void *) (shdr[i].sh_addr + baseAddress), shdr[i].sh_size);
        } else if (section_name[0] == '.' && section_name[1] == 's' && section_name[2] == 'b' && section_name[3] == 's' && section_name[4] == 's') {
            memset((void *) (shdr[i].sh_addr + baseAddress), 0, shdr[i].sh_size);
            DCFlushRange((void *) (shdr[i].sh_addr + baseAddress), shdr[i].sh_size);
        }
    }

    return ehdr->e_entry;
}
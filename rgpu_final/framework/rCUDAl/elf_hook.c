void *elf_hook(char const *module_filename, 
  void const *module_address, char const *name, void const *substitution)
{
    static size_t pagesize;

    int descriptor;  //file descriptor of shared module

    Elf_Shdr
    *dynsym = NULL,  // ".dynsym" section header
    *rel_plt = NULL,  // ".rel.plt" section header
    *rel_dyn = NULL;  // ".rel.dyn" section header

    Elf_Sym
    *symbol = NULL;  //symbol table entry for symbol named "name"

    Elf_Rel
    *rel_plt_table = NULL,  //array with ".rel.plt" entries
    *rel_dyn_table = NULL;  //array with ".rel.dyn" entries

    size_t
    i,
    name_index = 0,  //index of symbol named "name" in ".dyn.sym"
    rel_plt_amount = 0,  // amount of ".rel.plt" entries
    rel_dyn_amount = 0,  // amount of ".rel.dyn" entries
    *name_address = NULL;  //address of relocation for symbol named "name"

    void *original = NULL;  //address of the symbol being substituted

    if (NULL == module_address || NULL == name || NULL == substitution)
        return original;

    if (!pagesize)
        pagesize = sysconf(_SC_PAGESIZE);

    descriptor = open(module_filename, O_RDONLY);

    if (descriptor < 0)
        return original;

    if (
        section_by_type(descriptor, SHT_DYNSYM, &dynsym) ||  //get ".dynsym" section
        //actually, we need only the index of symbol named "name" in the ".dynsym" table
        symbol_by_name(descriptor, dynsym, name, &symbol, &name_index) ||
        //get ".rel.plt" (for 32-bit) or ".rela.plt" (for 64-bit) section
        section_by_name(descriptor, REL_PLT, &rel_plt) ||
        section_by_name(descriptor, REL_DYN, &rel_dyn)
        //get ".rel.dyn" (for 32-bit) or ".rela.dyn" (for 64-bit) section
       )
    {  //if something went wrong
        free(dynsym);
        free(rel_plt);
        free(rel_dyn);
        free(symbol);
        close(descriptor);

        return original;
    }
//release the data used
    free(dynsym);
    free(symbol);

    rel_plt_table = (Elf_Rel *)(((size_t)module_address) + rel_plt->sh_addr);  //init the ".rel.plt" array
    rel_plt_amount = rel_plt->sh_size / sizeof(Elf_Rel);  //and get its size

    rel_dyn_table = (Elf_Rel *)(((size_t)module_address) + rel_dyn->sh_addr);  //init the ".rel.dyn" array
    rel_dyn_amount = rel_dyn->sh_size / sizeof(Elf_Rel);  //and get its size
//release the data used
    free(rel_plt);
    free(rel_dyn);
//and descriptor
    close(descriptor);
//now we've got ".rel.plt" (needed for PIC) table
//and ".rel.dyn" (for non-PIC) table and the symbol's index
    for (i = 0; i < rel_plt_amount; ++i)  //lookup the ".rel.plt" table
        if (ELF_R_SYM(rel_plt_table[i].r_info) == name_index)
        //if we found the symbol to substitute in ".rel.plt"
        {
            original = (void *)*(size_t *)(((size_t)module_address) + 
              rel_plt_table[i].r_offset);  //save the original function address
            *(size_t *)(((size_t)module_address) + 
              rel_plt_table[i].r_offset) = (size_t)substitution;
              //and replace it with the substitutional

            break;  //the target symbol appears in ".rel.plt" only once
        }

    if (original)
        return original;
//we will get here only with 32-bit non-PIC module
    for (i = 0; i < rel_dyn_amount; ++i)  //lookup the ".rel.dyn" table
        if (ELF_R_SYM(rel_dyn_table[i].r_info) == name_index)
        //if we found the symbol to substitute in ".rel.dyn"
        {
            name_address = (size_t *)(((size_t)module_address) + rel_dyn_table[i].r_offset);
            //get the relocation address (address of a relative CALL (0xE8) instruction's argument)

            if (!original)
                original = (void *)(*name_address + (size_t)name_address + sizeof(size_t));
                //calculate an address of the original function by a relative CALL (0xE8) instruction's argument

            mprotect((void *)(((size_t)name_address) & (((size_t)-1) ^ (pagesize - 1))), 
              pagesize, PROT_READ | PROT_WRITE);  //mark a memory page that contains the relocation as writable

            if (errno)
                return NULL;

            *name_address = (size_t)substitution - (size_t)name_address - sizeof(size_t);
            //calculate a new relative CALL (0xE8) instruction's argument for the substitutional function and write it down

            mprotect((void *)(((size_t)name_address) & (((size_t)-1) ^ (pagesize - 1))), 
              pagesize, PROT_READ | PROT_EXEC);  //mark a memory page that contains the relocation back as executable

            if (errno)  //if something went wrong
            {
                *name_address = (size_t)original - 
                  (size_t)name_address - sizeof(size_t);  //then restore the original function address

                return NULL;
            }
        }

    return original;
}


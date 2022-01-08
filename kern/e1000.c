#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t *e1000;

int pci_e1000_attach(struct pci_func *pcif)
{
    pci_func_enable(pcif);

    e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("e1000: bar0 %x size0 %x\n", pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("e1000: status %x\n", e1000[E1000_STATUS/4]);

    return 1;
}

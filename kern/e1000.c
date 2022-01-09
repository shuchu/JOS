#include <inc/x86.h>
#include <inc/string.h>
#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t *e1000;
struct tx_desc tx_d[TXRING_LEN] __attribute__((aligned (PGSIZE)))
       = {{0, 0, 0, 0, 0, 0, 0}};
struct packet pbuf[TXRING_LEN] __attribute__((aligned (PGSIZE)))
       = {{{0}}};

static void
init_desc() {
    int i;

    for (i = 0; i < TXRING_LEN; i++) {
        memset(&tx_d[i], 0, sizeof(tx_d[i]));
        tx_d[i].addr = PADDR(&pbuf[i]);
        tx_d[i].status = E1000_TXD_STAT_DD;
        tx_d[i].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
    }
}



int pci_e1000_attach(struct pci_func *pcif)
{
    pci_func_enable(pcif);
    init_desc();

    e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("e1000: bar0 %x size0 %x\n", pcif->reg_base[0], pcif->reg_size[0]);
    
    e1000[E1000_TDBAL/4] = PADDR(tx_d);
    e1000[E1000_TDBAH/4] = 0;
    e1000[E1000_TDLEN/4] = TXRING_LEN * sizeof(struct tx_desc);
    e1000[E1000_TDH/4] = 0;
    e1000[E1000_TDT/4] = 0;
    e1000[E1000_TCTL/4] = E1000_TCTL_EN | 
                          E1000_TCTL_PSP |
                          (E1000_TCTL_CT & (0x10 << 4)) |
                          (E1000_TCTL_COLD & (0x40 << 12));
    e1000[E1000_TIPG/4] = 10 | (8 << 10) | (12 << 20);

    cprintf("e1000: status %x\n", e1000[E1000_STATUS/4]);

    return 1;
}

int
e1000_transmit(void *addr, size_t len)
{
    uint32_t tail = e1000[E1000_TDT/4];
    struct tx_desc *nxt = &tx_d[tail];

    if ((nxt->status & E1000_TXD_STAT_DD) != E1000_TXD_STAT_DD)
        return -1;
    if (len > TBUFFSIZE)
        len = TBUFFSIZE;
    
    memmove(&pbuf[tail].body, addr, len);
    nxt->length = (uint16_t) len;
    nxt->status &= !E1000_TXD_STAT_DD;
    e1000[E1000_TDT/4] = (tail+1) % TXRING_LEN;
    return 0;

}

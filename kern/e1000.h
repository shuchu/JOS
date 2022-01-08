#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define PCI_E1000_VENDOR 0x8086
#define PCI_E1000_DEVICE 0x100E

#define E1000_STATUS   0x00008  /* Device Status - RO */

int pci_e1000_attach(struct pci_func *pcif);


#endif  // SOL >= 6

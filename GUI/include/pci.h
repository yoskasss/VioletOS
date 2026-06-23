/* VioletOS - PCI config space */
#ifndef VIOLET_PCI_H
#define VIOLET_PCI_H

#include "types.h"

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint8_t  pci_read8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
bool pci_find_device(uint16_t vendor, uint16_t device, uint8_t* bus, uint8_t* slot, uint8_t* func);

#endif /* VIOLET_PCI_H */

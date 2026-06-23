/* VioletOS - PCI configuration access */
#include "pci.h"
#include "port.h"

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = 0x80000000u | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                    ((uint32_t)func << 8) | (offset & 0xFCu);
    outl(0xCF8, addr);
    return inl(0xCFC);
}

uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t v = pci_read32(bus, slot, func, offset & 0xFCu);
    return (uint16_t)(v >> ((offset & 2) * 8));
}

uint8_t pci_read8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t v = pci_read32(bus, slot, func, offset & 0xFCu);
    return (uint8_t)(v >> ((offset & 3) * 8));
}

bool pci_find_device(uint16_t vendor, uint16_t device, uint8_t* bus, uint8_t* slot, uint8_t* func) {
    for (uint16_t s = 0; s < 32; s++) {
        uint16_t vid = pci_read16(0, (uint8_t)s, 0, 0);
        if (vid == 0xFFFF) continue;
        uint16_t did = pci_read16(0, (uint8_t)s, 0, 2);
        if (vid == vendor && did == device) {
            if (bus) *bus = 0;
            if (slot) *slot = (uint8_t)s;
            if (func) *func = 0;
            return true;
        }
    }
    return false;
}

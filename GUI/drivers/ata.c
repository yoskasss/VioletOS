/* VioletOS - ATA PIO disk driver for persistent storage */
#include "violet.h"
#include "port.h"

#define ATA_PRIMARY_DATA    0x1F0
#define ATA_PRIMARY_ERROR   0x1F1
#define ATA_PRIMARY_SECTORS 0x1F2
#define ATA_PRIMARY_LBA_LO  0x1F3
#define ATA_PRIMARY_LBA_MID 0x1F4
#define ATA_PRIMARY_LBA_HI  0x1F5
#define ATA_PRIMARY_DRIVE   0x1F6
#define ATA_PRIMARY_STATUS  0x1F7
#define ATA_PRIMARY_CMD     0x1F7

#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF   0x20
#define ATA_SR_ERR  0x01

#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30

static bool ata_available = false;

static void ata_wait_bsy(void) {
    while (inb(ATA_PRIMARY_STATUS) & ATA_SR_BSY);
}

static void ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_STATUS) & 0x08));
}

void ata_init(void) {
    outb(ATA_PRIMARY_DRIVE, 0xE0);
    io_wait();
    uint8_t status = inb(ATA_PRIMARY_STATUS);
    ata_available = !(status == 0xFF || status == 0x00);
}

bool ata_read_sector(uint32_t lba, uint8_t* buffer) {
    if (!ata_available) return false;

    ata_wait_bsy();
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECTORS, 1);
    outb(ATA_PRIMARY_LBA_LO,  (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI,  (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_CMD, ATA_CMD_READ_SECTORS);

    ata_wait_bsy();
    if (inb(ATA_PRIMARY_STATUS) & ATA_SR_ERR) return false;
    ata_wait_drq();

    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(ATA_PRIMARY_DATA);
        buffer[i * 2]     = (uint8_t)(data & 0xFF);
        buffer[i * 2 + 1] = (uint8_t)((data >> 8) & 0xFF);
    }
    return true;
}

bool ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    if (!ata_available) return false;

    ata_wait_bsy();
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECTORS, 1);
    outb(ATA_PRIMARY_LBA_LO,  (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI,  (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_CMD, ATA_CMD_WRITE_SECTORS);

    ata_wait_bsy();
    ata_wait_drq();

    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        outw(ATA_PRIMARY_DATA, data);
    }
    outb(ATA_PRIMARY_CMD, 0xE7); /* Flush cache */
    ata_wait_bsy();
    return true;
}

/* VioletOS - RTL8139 PCI ethernet driver (polling mode) */
#include "violet.h"
#include "pci.h"
#include "port.h"

#define RTL8139_VENDOR 0x10EC
#define RTL8139_DEVICE 0x8139

#define RTL_CMD      0x37
#define RTL_RXBUF    0x30
#define RTL_CAPR     0x38
#define RTL_CBR      0x3A
#define RTL_ISR      0x3E
#define RTL_TSD0     0x10
#define RTL_TSAD0    0x20
#define RTL_IDR0     0x00

static uint16_t rtl_io = 0;
static bool rtl_ok = false;
static uint8_t rtl_mac[6] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
static uint8_t rx_buf[8192 + 16 + 1500] __attribute__((aligned(256)));
static uint8_t tx_buf[1536];

static uint8_t rtl_in8(uint16_t reg) {
    return inb((uint16_t)(rtl_io + reg));
}

static void rtl_out8(uint16_t reg, uint8_t val) {
    outb((uint16_t)(rtl_io + reg), val);
}

static void rtl_out16(uint16_t reg, uint16_t val) {
    outw((uint16_t)(rtl_io + reg), val);
}

static uint16_t rtl_in16(uint16_t reg) {
    return inw((uint16_t)(rtl_io + reg));
}

static void rtl_out32(uint16_t reg, uint32_t val) {
    outl((uint16_t)(rtl_io + reg), val);
}

static uint32_t rtl_in32(uint16_t reg) {
    return inl((uint16_t)(rtl_io + reg));
}

bool rtl8139_init(void) {
    uint8_t bus, slot, func;
    if (!pci_find_device(RTL8139_VENDOR, RTL8139_DEVICE, &bus, &slot, &func))
        return false;

    uint32_t bar = pci_read32(bus, slot, func, 0x10);
    if (bar & 1)
        rtl_io = (uint16_t)(bar & 0xFFFC);
    else
        return false;

    rtl_out8(RTL_CMD, 0x10);
    for (int i = 0; i < 100000; i++) {
        if (!(rtl_in8(RTL_CMD) & 0x10)) break;
    }

    rtl_out32(RTL_RXBUF, (uint32_t)rx_buf);
    rtl_out32(RTL_ISR, 0xFFFF);

    for (int i = 0; i < 6; i++)
        rtl_out8((uint16_t)(RTL_IDR0 + i), rtl_mac[i]);

    rtl_out8(RTL_CMD, 0x0C); /* RX+TX enable */

    rtl_ok = true;
    return true;
}

bool rtl8139_ready(void) {
    return rtl_ok;
}

const uint8_t* rtl8139_mac(void) {
    return rtl_mac;
}

bool rtl8139_send(const void* data, uint16_t len) {
    if (!rtl_ok || len > 1514) return false;

    memcpy(tx_buf, data, len);
    rtl_out32(RTL_TSAD0, (uint32_t)tx_buf);
    rtl_out32(RTL_TSD0, len);

    for (int t = 0; t < 500000; t++) {
        uint32_t status = rtl_in32(RTL_TSD0);
        if (!(status & 0x2000)) return true;
        if (status & 0x8000) return false;
    }
    return false;
}

bool rtl8139_poll(void* out, uint16_t* len, uint16_t max) {
    if (!rtl_ok) return false;

    uint16_t capr = rtl_in16(RTL_CAPR);
    uint16_t cbr  = rtl_in16(RTL_CBR);

    if (capr == cbr) return false;

    uint16_t offset = (uint16_t)(capr % 8192);
    uint16_t* hdr = (uint16_t*)(rx_buf + offset);
    uint16_t status = hdr[0];
    uint16_t plen = hdr[1];

    if (!(status & 0x01) || plen == 0 || plen > max) {
        capr = (uint16_t)((capr + 4 + 8191) & ~8191);
        rtl_out16(RTL_CAPR, capr);
        return false;
    }

    plen = (uint16_t)(plen - 4);
    if (plen > max) plen = max;
    memcpy(out, rx_buf + offset + 4, plen);
    *len = plen;

    capr = (uint16_t)((capr + plen + 4 + 3) & ~3);
    rtl_out16(RTL_CAPR, capr);
    return true;
}

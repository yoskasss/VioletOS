/* VioletOS - Programmable Interval Timer driver */
#include "violet.h"
#include "port.h"

static volatile uint32_t timer_ticks = 0;

void timer_handler(void) {
    timer_ticks++;
    pic_eoi(0);
}

void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

uint32_t timer_get_uptime_seconds(void) {
    return timer_ticks / 100;
}

#include "timer.h"
#include "ports.h"

#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40
#define PIT_BASE_FREQUENCY 1193182

static uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 0;

void timer_init(uint32_t frequency_hz) {
    if (frequency_hz == 0) {
        frequency_hz = 100; // If zero default to 100 Hz
    }
    timer_frequency = frequency_hz;
    uint32_t divisor = PIT_BASE_FREQUENCY / timer_frequency;
    if (divisor == 0) {
        divisor = 1;
    }
    outb(PIT_COMMAND_PORT, 0x36);
    outb(PIT_CHANNEL0_PORT, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_PORT, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_tick(void) {
    timer_ticks++;
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

uint32_t timer_get_frequency(void) {
    return timer_frequency;
}
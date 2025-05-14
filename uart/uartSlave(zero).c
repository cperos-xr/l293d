#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "ws2812.pio.h"

#define UART_ID        uart0
#define BAUD_RATE      9600
#define UART_TX_PIN    28
#define UART_RX_PIN    29

#define RGB_PIN        16
#define PACKET_LEN     10

#define LED_HOLD_MS    500

// GRB order: your LED expects green first, then red, then blue
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

#define LED_GOOD  urgb_u32(0, 255, 0)   // Green
#define LED_ERROR urgb_u32(255, 0, 0)   // Red
#define LED_IDLE  urgb_u32(16, 16, 16)  // Dim white

PIO pio = pio0;
int sm = 0;

// LED state
uint32_t led_active_color = 0;
absolute_time_t led_expire_time;
bool led_active = false;

void show_color(uint32_t color) {
    pio_sm_put_blocking(pio, sm, color << 8u);
}

void trigger_led(uint32_t color) {
    show_color(color);
    led_active_color = color;
    led_expire_time = make_timeout_time_ms(LED_HOLD_MS);
    led_active = true;
}

void check_led_timeout() {
    if (led_active && absolute_time_diff_us(get_absolute_time(), led_expire_time) < 0) {
        show_color(LED_IDLE);  // return to idle color
        led_active = false;
    }
}

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);

    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, RGB_PIN, 800000, false);

    led_active_color = LED_IDLE;
    show_color(led_active_color);

    while (true) {
        check_led_timeout();

        if (!uart_is_readable(UART_ID)) continue;

        uint8_t byte = uart_getc(UART_ID);
        if (byte != 0xAA) continue;

        // Build packet
        uint8_t packet[10];
        packet[0] = byte;
        int count = 1;
        absolute_time_t start = get_absolute_time();

        while (count < 10 && absolute_time_diff_us(get_absolute_time(), start) < 100000) {
            if (uart_is_readable(UART_ID)) {
                packet[count++] = uart_getc(UART_ID);
            }
        }

        if (count < 10) {
            uart_puts(UART_ID, "ERROR: Packet timeout/incomplete\n");
            trigger_led(LED_ERROR);
            continue;
        }

        // Validate checksum
        uint8_t checksum = 0;
        for (int i = 1; i <= 8; i++) checksum ^= packet[i];

        if (checksum != packet[9]) {
            uart_puts(UART_ID, "ERROR: Bad checksum\n");
            trigger_led(LED_ERROR);
            continue;
        }

        // Valid packet
        uart_puts(UART_ID, "[MOTORS] ");
        for (int i = 1; i <= 8; i++) {
            char out[6];
            snprintf(out, sizeof(out), "%d ", (int8_t)packet[i]);
            uart_puts(UART_ID, out);
        }
        uart_putc(UART_ID, '\n');
        trigger_led(LED_GOOD);
    }
}

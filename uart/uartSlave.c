#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "ws2812.pio.h"


#define UART_ID        uart0
#define BAUD_RATE      9600
#define UART_TX_PIN    28
#define UART_RX_PIN    29

#define RGB_PIN        16
#define PACKET_LEN     10


/////////////////////////////////////////////////////////
// Define motor pin mappings using arrays
const uint8_t motor_fwd_pins[] = {0, 2, 4, 6, 8, 10, 12, 14};
const uint8_t motor_rev_pins[] = {1, 3, 5, 7, 9, 11, 13, 15};
/////////////////////////////////////////////////////////


#define LED_HOLD_MS    500

// GRB order: your LED expects green first, then red, then blue
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

#define LED_GOOD  urgb_u32(0, 255, 0)   // Green
#define LED_ERROR urgb_u32(255, 0, 0)   // Red
#define LED_IDLE  urgb_u32(16, 16, 16)  // Dim white

PIO pio = pio0;
int sm = 0;

int count = 0;

// LED state
uint32_t led_active_color = 0;
absolute_time_t led_expire_time;
bool led_active = false;

void show_color(uint32_t color)
{
    pio_sm_put_blocking(pio, sm, color << 8u);
}

void trigger_led(uint32_t color)
{
    show_color(color);
    led_active_color = color;
    led_expire_time = make_timeout_time_ms(LED_HOLD_MS);
    led_active = true;
}

void check_led_timeout()
{
    if (led_active && absolute_time_diff_us(get_absolute_time(), led_expire_time) < 0) {
        show_color(LED_IDLE);  // return to idle color
        led_active = false;
    }
}

void process_motor_value(int index, int8_t value)
{
    char out[16];
    snprintf(out, sizeof(out), "M%d: %d ", index, value);
    uart_puts(UART_ID, out);
}

void activate_corresponding_motor_pin(int motor_index, int8_t value)
{
    if (motor_index < 1 || motor_index > 8) return;

    uint8_t fwd_pin = motor_fwd_pins[motor_index - 1];
    uint8_t rev_pin = motor_rev_pins[motor_index - 1];

    if (value > 0) {
        pwm_set_gpio_level(fwd_pin, value * 2);   // up to 254
        pwm_set_gpio_level(rev_pin, 0);
    } else if (value < 0) {
        pwm_set_gpio_level(fwd_pin, 0);
        pwm_set_gpio_level(rev_pin, (-value) * 2);
    } else {
        pwm_set_gpio_level(fwd_pin, 0);
        pwm_set_gpio_level(rev_pin, 0);
    }
}

void initialize_motor_pins()
{
    for (int i = 0; i < 8; i++)
    {
    gpio_set_function(motor_fwd_pins[i], GPIO_FUNC_PWM);
    gpio_set_function(motor_rev_pins[i], GPIO_FUNC_PWM);

    uint slice_fwd = pwm_gpio_to_slice_num(motor_fwd_pins[i]);
    uint slice_rev = pwm_gpio_to_slice_num(motor_rev_pins[i]);

    pwm_set_wrap(slice_fwd, 255);
    pwm_set_wrap(slice_rev, 255);

    pwm_set_enabled(slice_fwd, true);
    pwm_set_enabled(slice_rev, true);
    }

}

int main()
{
    stdio_init_all();
    initialize_motor_pins();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);

    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, RGB_PIN, 800000, false);

    led_active_color = LED_IDLE;
    show_color(led_active_color);

    while (true)
    {
        check_led_timeout();

        if (!uart_is_readable(UART_ID)) continue;

        uint8_t byte = uart_getc(UART_ID);
        if (byte != 0xAA) continue;

        // Build packet
        uint8_t packet[10];
        packet[0] = byte;
        int count = 1;
        absolute_time_t start = get_absolute_time();

        while (count < 10 && absolute_time_diff_us(get_absolute_time(), start) < 100000)
        {
            if (uart_is_readable(UART_ID)) 
            {
                packet[count++] = uart_getc(UART_ID);
            }
        }

        if (count < 10) 
        {
            uart_puts(UART_ID, "ERROR: Packet timeout/incomplete\n");
            trigger_led(LED_ERROR);
            continue;
        }

        // Validate checksum
        uint8_t checksum = 0;
        for (int i = 1; i <= 8; i++) checksum ^= packet[i];

        if (checksum != packet[9])
        {
            uart_puts(UART_ID, "ERROR: Bad checksum\n");
            trigger_led(LED_ERROR);
            continue;
        }

        // Valid packet
        uart_puts(UART_ID, "[MOTORS] ");
        for (int i = 1; i <= 8; i++)
        {
            process_motor_value(i, (int8_t)packet[i]);
            activate_corresponding_motor_pin(i, (int8_t)packet[i]);
        }
        uart_putc(UART_ID, '\n');
        trigger_led(LED_GOOD);
    }


}

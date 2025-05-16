#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/stdio_usb.h"
#include "PCA9685_servo_controller.h"
#include "PCA9685_servo.h"

#define UART_ID        uart0
#define BAUD_RATE      9600
#define UART_TX_PIN    0
#define UART_RX_PIN    1

// ——— Motor packet helper ———
void send_motor_packet(int8_t motors[8]) {
    uint8_t packet[10];
    packet[0] = 0xAA;
    uint8_t checksum = 0;
    for (int i = 0; i < 8; i++) {
        packet[i + 1] = (uint8_t)motors[i];
        checksum ^= packet[i + 1];
    }
    packet[9] = checksum;
    uart_write_blocking(UART_ID, packet, 10);
}

// ——— PCA9685 + 4 servos on channels 0–3 ———
PCA9685_servo_controller servoController(i2c0, 12, 13, 0x40);
PCA9685_servo servos[4] = {
    PCA9685_servo(&servoController, 0),
    PCA9685_servo(&servoController, 1),
    PCA9685_servo(&servoController, 2),
    PCA9685_servo(&servoController, 3)
};

int main() {
    // —— Init USB-CDC & UART —
    stdio_init_all();
    while (!stdio_usb_connected()) tight_loop_contents();
    printf("Ready: send 4, 8 or 12 values:\n"
           " • 4 → servos (–90…+90)\n"
           " • 8 → motors (–128…127)\n"
           " • 12 → first 8 motors, last 4 servos\n");

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);

    // —— Init I²C @ 400 kHz + PCA9685 —
    i2c_init(i2c0, 400000);
    gpio_set_function(12, GPIO_FUNC_I2C);
    gpio_set_function(13, GPIO_FUNC_I2C);

    servoController.begin();
    servoController.setFrequency(50);

    for (int i = 0; i < 4; ++i) {
        servos[i].setRange(-90, 90);
        servos[i].setMode(MODE_FAST);    // full-speed “as-fast-as-possible”
        servos[i].setPosition(0);        // center at 0°
    }

    // —— Main loop: blocking fgets is fine in FAST mode —
    const int MAX_INPUT = 128;
    char input[MAX_INPUT];

    while (true) {
        if (!fgets(input, sizeof(input), stdin)) {
            clearerr(stdin);
            continue;
        }

        // Tokenize into up to 12 ints
        int tokens[12];
        int count = 0;
        char *tok = strtok(input, " \t\r\n");
        while (tok && count < 12) {
            tokens[count++] = atoi(tok);
            tok = strtok(NULL, " \t\r\n");
        }

        if (count == 4 || count == 8 || count == 12) {
            // —— Motors? send if 8 or 12 tokens —
            if (count >= 8) {
                int8_t motors[8];
                for (int i = 0; i < 8; ++i) {
                    int v = tokens[i];
                    if (v < -128) v = -128;
                    if (v > +127) v = +127;
                    motors[i] = (int8_t)v;
                }
                send_motor_packet(motors);
                printf("✅ Motors: ");
                for (int i = 0; i < 8; ++i) printf("%d ", motors[i]);
                printf("\n");

                // optional: wait for echo
                char echo[64];
                int pos = 0;
                uint64_t t0 = time_us_64();
                while (time_us_64() - t0 < 1000000) {
                    if (uart_is_readable(UART_ID)) {
                        char c = uart_getc(UART_ID);
                        if (c == '\n' || pos >= (int)sizeof(echo)-1) break;
                        echo[pos++] = c;
                    }
                }
                if (pos) {
                    echo[pos] = '\0';
                    printf("🔁 Echo: %s\n", echo);
                } else {
                    printf("⚠️ No slave response\n");
                }
            }

            // —— Servos? apply if 4 or 12 tokens —
            if (count == 4 || count == 12) {
                int base = (count == 12 ? 8 : 0);
                printf("✅ Servos: ");
                for (int i = 0; i < 4; ++i) {
                    int angle = tokens[base + i];
                    if (angle < -90)  angle = -90;
                    if (angle > +90)  angle = +90;
                    servos[i].setPosition(angle);
                    printf("%d ", angle);
                }
                printf("\n");
            }
        } else {
            printf("❌ Need 4, 8 or 12 ints; got %d\n", count);
        }
    }

    return 0;
}

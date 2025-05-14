#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/stdio_usb.h"

#define UART_ID        uart0
#define BAUD_RATE      9600
#define UART_TX_PIN    0
#define UART_RX_PIN    1
#define MAX_INPUT_LEN  128

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

int main() {
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);

    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }

    printf("Motor packet sender ready.\n");
    printf("Type 8 numbers from -128 to 127 separated by space:\n");

    char input[MAX_INPUT_LEN];
    while (true) {
        fgets(input, sizeof(input), stdin);

        // Parse input into motor values
        int8_t motors[8];
        char* token = strtok(input, " ");
        int count = 0;

        while (token && count < 8) {
            int val = atoi(token);
            if (val < -128) val = -128;
            if (val > 127)  val = 127;
            motors[count++] = (int8_t)val;
            token = strtok(NULL, " ");
        }

        if (count < 8) {
            printf("Please enter exactly 8 values.\n");
            continue;
        }

        send_motor_packet(motors);
        printf("✅ Packet sent: ");
        for (int i = 0; i < 8; i++) {
            printf("%d ", motors[i]);
        }
        printf("\n");

        // Optionally: wait for echo from slave
        char echo[128];
        int pos = 0;
        uint64_t start = time_us_64();
        while (time_us_64() - start < 1000000) {
            if (uart_is_readable(UART_ID)) {
                char c = uart_getc(UART_ID);
                if (c == '\n' || pos >= sizeof(echo) - 1) break;
                echo[pos++] = c;
            }
        }
        echo[pos] = '\0';
        if (pos > 0) {
            printf("Slave says: %s\n", echo);
        } else {
            printf("No response from slave.\n");
        }
    }
}

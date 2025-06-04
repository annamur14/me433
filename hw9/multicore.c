/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#define ADC0_PIN 26
#define LED_PIN 15

typedef enum
{
    GET_VOLTAGE = 0,
    LED_ON = 1,
    LED_OFF = 2,
} command_t;

float adc_to_volts(uint16_t adc_value)
{
    return (adc_value * 3.3f) / 4095.0f;
}

void core1_entry()
{
    adc_init();
    adc_gpio_init(ADC0_PIN);
    adc_select_input(0);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (1)
    {
        uint32_t command = multicore_fifo_pop_blocking();

        if (command == GET_VOLTAGE)
        {
            uint16_t adc_value = adc_read();
            multicore_fifo_push_blocking((uint32_t)adc_value);
        }
        else if (command == LED_ON)
        {
            gpio_put(LED_PIN, 1);
        }
        else if (command == LED_OFF)
        {
            gpio_put(LED_PIN, 0);
        }
    }
}

int main()
{
    stdio_init_all();
    printf("Hello, multicore!\n");

    while (!stdio_usb_connected())
    {
        sleep_ms(100);
    }

    /// \tag::setup_multicore[]

    multicore_launch_core1(core1_entry);

    // Wait for it to start up

    while (true)
    {
        int flag = 0;
        char message[100];

        printf("Enter command (0 = GET_VOLTAGE, 1 = LED_ON, 2 = LED_OFF): ");
        fflush(stdout);

        if (scanf("%d", &flag) != 1)
        {
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
            {
            }
            printf("  → invalid input, please type 0, 1, or 2.\n");
            continue;
        }

        snprintf(message, sizeof(message), "%d\n", flag);
        printf("%s", message);

        multicore_fifo_push_blocking((uint32_t)flag);

        if (flag == GET_VOLTAGE)
        {
            uint32_t adc_value = multicore_fifo_pop_blocking();
            float volts = adc_to_volts((uint16_t)adc_value);
            snprintf(message, sizeof(message), "ADC0 Voltage: %f V\n", volts);
            printf("%s", message);
        }
    }

    /// \end::setup_multicore[]
}

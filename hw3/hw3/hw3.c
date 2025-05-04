#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 15
#define BUTTON_PIN 2

int main()
{
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        sleep_ms(100);
    }
    printf("Start!\n");

    gpio_init(15);
    gpio_init(2);

    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);

    while (1)
    {
        if (gpio_get(BUTTON_PIN))
        {
            gpio_put(LED_PIN, 1);
            printf("LED OFF\n");
        }
        else
        {
            gpio_put(LED_PIN, 0);
            printf("LED ON\n");
        }
        // char message[100];
        // scanf("%s", message);
        // printf("message: %s\r\n", message);
        // sleep_ms(50);
    }
}
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LED_PIN 15
#define BUTTON_PIN 2

void init_gpios()
{
    gpio_init(15);
    gpio_init(2);

    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
}

void init_usb_port()
{
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        sleep_ms(100);
    }
    printf("Start!\n");
}

void poll_button()
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
}

int main()
{
    init_usb_port();
    init_gpios();

    while (1)
    {
        poll_button();
        // char message[100];
        // scanf("%s", message);
        // printf("message: %s\r\n", message);
        // sleep_ms(50);
    }
}
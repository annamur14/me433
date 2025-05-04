#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LED_PIN 15
#define BUTTON_PIN 2

// uint8_t num_samples;

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

void init_adc()
{
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
}

void init()
{
    init_usb_port();
    init_gpios();
    init_adc();

    gpio_put(LED_PIN, 0);
    while (gpio_get(BUTTON_PIN))
    { // unpressed
        gpio_put(LED_PIN, 1);
        printf("LED off, in init\n");
        // sleep_ms(100);
    }
}

int main()
{
    init();

    while (1)
    {
        int num_samples = 0;
        printf("\nEnter number of analog samples to take (1-100): ");
        scanf("%d", &num_samples);
        // read_adc();
        // printf("in read_adc\n");
        float adc_samples[num_samples];
        for (int i = 0; i < num_samples; i++)
        {
            // printf("in read_adc for loop\n");
            uint16_t result = adc_read();
            float voltage = (result * 3.3f) / 4095.0f;
            adc_samples[i] = voltage;
            sleep_ms(10);
        }
        for (int i = 0; i < num_samples; i++)
        {
            printf("\nADC0 voltage %d: %f V", i, adc_samples[i]);
        }
        // sleep_ms(1000);
    }
}
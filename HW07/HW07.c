#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "font.h"
#include "ssd1306.h"
#include "hardware/adc.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define ADC0_PIN 26
#define ADC0 0

#define ONBOARD_LED 25

void heartbeat()
{
    static uint16_t i = 0;
    i = (i + 1) % 100;
    if (i == 0)
    {
        gpio_put(ONBOARD_LED, 0);
    }
    else if (i == 50)
    {
        gpio_put(ONBOARD_LED, 1);
    }
}

void draw_char(char c, int x, int y)
{
    for (int i = 0; i < 5; i++)
    {
        unsigned char line = ASCII[c - 32][i];
        for (int j = 0; j < 8; j++)
        {
            if (line & (1 << j))
            {
                ssd1306_drawPixel(x + i, y + j, 1); // Draw pixel
            }
            else
            {
                ssd1306_drawPixel(x + i, y + j, 0); // Clear pixel
            }
        }
    }
}

void draw_string(char *str, int x, int y)
{
    while (*str)
    {
        draw_char(*str++, x, y);
        x += 6; // Move to the next character position (5 pixels + 1 pixel space)
    }
}

float adc_to_volts(uint16_t adc_value)
{
    return (adc_value * 3.3f) / 4095.0f;
}

int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    gpio_init(ONBOARD_LED);
    gpio_set_dir(ONBOARD_LED, GPIO_OUT);

    ssd1306_setup();
    sleep_ms(100);

    adc_init();
    adc_gpio_init(ADC0_PIN);
    adc_select_input(ADC0);

    char str[50];

    while (true)
    {
        heartbeat();

        draw_char('H', 0, 0);

        sprintf(str, "ADC0: %f V", adc_to_volts(adc_read()));
        draw_string(str, 0, 10);

        ssd1306_update();
    }
}

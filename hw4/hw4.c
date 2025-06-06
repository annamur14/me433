#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS 17
#define PIN_SCK 18
#define PIN_MOSI 19

static inline void cs_select(uint cs_pin)
{
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin)
{
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}

void send_data(uint8_t channel, float value)
{
    if (value < 0)
    {
        value = 0;
    }
    else if (value > 3.3)
    {
        value = 3.3;
    }

    uint16_t value_int = (uint16_t)((value / 3.3f) * 1023.0f); // 10 bit

    uint8_t data[2] = {0, 0};
    data[0] = data[0] | (channel << 7) | (7 << 4) | (value_int >> 6);
    data[1] = data[1] | (value_int << 2);

    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, sizeof(data));
    cs_deselect(PIN_CS);
}

int main()
{
    stdio_init_all();

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi

    while (true)
    {
        // send_data(0, 0.5f); // Channel 0, 0.5V
        // send_data(1, 2.0f); // Channel 1, 2.0V
        float value = 0.0f;
        float time = 0.0f;
        for (int i = 0; i < 200; i++)
        {
            value = 1.65 * sin(2 * time * 2.0f * M_PI) + 1.65;
            send_data(0, value);
            value = fabs(3.3 * (fmod(2 * time, 2) - 1));
            send_data(1, value);

            time += 0.002;

            sleep_ms(2);
        }

        sleep_ms(10);
    }
}

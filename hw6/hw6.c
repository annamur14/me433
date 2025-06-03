#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines

#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define MCP_ADDR 0b100000

#define IODIR 0x00
#define IPOL 0x01
#define GPINTEN 0x02
#define DEFVAL 0x03
#define INTCON 0x04
#define IOCON 0x05
#define GPPU 0x06
#define INTF 0x07
#define INTCAP 0x08
#define GPIO 0x09
#define OLAT 0x0A

#define ONBOARD_LED 25

void heartbeat(void);

void write_pin(uint8_t pin, bool val)
{
    printf("Writing pin %d with value %d\n", pin, val);
    uint8_t buf[2] = {GPIO, val << (pin)};
    i2c_write_blocking(I2C_PORT, MCP_ADDR, buf, 2, false);
}

uint8_t read_pin(uint8_t pin)
{
    printf("Reading pin %d\n", pin);
    uint8_t buf;
    uint8_t reg = GPIO;

    i2c_write_blocking(I2C_PORT, MCP_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MCP_ADDR, &buf, 1, false);

    return buf & (1 << pin);
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

    uint8_t buf[2] = {IODIR, 0b01111111};
    i2c_write_blocking(I2C_PORT, MCP_ADDR, buf, 2, false);

    while (true)
    {
        heartbeat();
        sleep_ms(5);
        write_pin(7, !read_pin(0));
    }
}

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
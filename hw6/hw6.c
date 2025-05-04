#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1

// MCP23008 defines
#define MCP23008_ADDR 0b100000 // Address with A0, A1, A2 connected to GND
#define IODIR 0x00             // I/O Direction register
#define GPIO 0x09              // GPIO Port register (read pin values)
#define OLAT 0x0A              // Output Latch register (set output pins)

// Pico built-in LED
#define LED_PIN 25

// Function prototypes
void setPin(uint8_t address, uint8_t reg, uint8_t value);
uint8_t readPin(uint8_t address, uint8_t reg);
void init_mcp23008(void);

void init_usb_port()
{
    stdio_init_all();
    while (!stdio_usb_connected())
    {
        sleep_ms(100);
    }
    printf("Start!\n");
}

int main()
{
    init_usb_port();
    printf("hello\n");

    // I2C Initialization at 400KHz
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    // gpio_pull_up(I2C_SDA);
    // gpio_pull_up(I2C_SCL);

    // Initialize built-in LED for heartbeat
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Initialize the MCP23008 chip
    init_mcp23008();

    printf("MCP23008 I/O expander example started\n");

    // Variables for heartbeat and button state
    bool heartbeat = false;
    bool prev_button_state = false;

    while (true)
    {
        // Toggle heartbeat LED
        heartbeat = !heartbeat;
        gpio_put(LED_PIN, heartbeat);

        // Read button state from GP0
        uint8_t gpio_value = readPin(MCP23008_ADDR, GPIO);
        bool button_pressed = !(gpio_value & 0x01); // GP0 is active low with pull-up

        // Control LED on GP7 based on button state
        uint8_t olat_value = readPin(MCP23008_ADDR, OLAT);

        if (button_pressed)
        {
            // Turn ON GP7 (set bit 7)
            setPin(MCP23008_ADDR, OLAT, olat_value | (1 << 7));
        }
        else
        {
            // Turn OFF GP7 (clear bit 7)
            setPin(MCP23008_ADDR, OLAT, olat_value & ~(1 << 7));
        }

        // Print state changes
        if (button_pressed != prev_button_state)
        {
            printf("Button %s, LED %s\n",
                   button_pressed ? "PRESSED" : "UNPRESSED",
                   button_pressed ? "OFF" : "ON");
            prev_button_state = button_pressed;
        }

        // Heartbeat indicator
        if (heartbeat)
        {
            printf("♥\n");
        }

        sleep_ms(250); // Blink frequency for heartbeat
    }
}

// Initialize the MCP23008
void init_mcp23008(void)
{
    // Configure GP7 as output (0) and all other pins as inputs (1)
    // IODIR: 0 = output, 1 = input
    // 0x7F = 0b01111111 (GP7=output, GP0-GP6=inputs)
    setPin(MCP23008_ADDR, IODIR, 0x7F);

    // Initialize output latch - all pins off
    setPin(MCP23008_ADDR, OLAT, 0x00);

    printf("MCP23008 initialized\n");
}

// Write a value to a register
void setPin(uint8_t address, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, address, data, 2, false);
}

// Read a value from a register
uint8_t readPin(uint8_t address, uint8_t reg)
{
    uint8_t result;

    // First write the register we want to read
    i2c_write_blocking(I2C_PORT, address, &reg, 1, true); // true to keep control of bus

    // Then read from that register
    i2c_read_blocking(I2C_PORT, address, &result, 1, false);

    return result;
}
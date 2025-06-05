#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "font.h"
#include "ssd1306.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5

#define ONBOARD_LED 25

// config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
// sensor data registers:
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H 0x41
#define TEMP_OUT_L 0x42
#define GYRO_XOUT_H 0x43
#define GYRO_XOUT_L 0x44
#define GYRO_YOUT_H 0x45
#define GYRO_YOUT_L 0x46
#define GYRO_ZOUT_H 0x47
#define GYRO_ZOUT_L 0x48
#define WHO_AM_I 0x75
// MPU6050 I2C address
#define MPU6050_ADDR 0x68

// directions for drawing lines
#define X_DIR 0
#define Y_DIR 1

#define X_MAX 128
#define Y_MAX 32

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

void get_mpu_data(float *data)
{
    uint8_t raw_data[6];
    uint8_t reg = ACCEL_XOUT_H;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, raw_data, 6, false);

    int16_t accel_x = (raw_data[0] << 8) | raw_data[1];
    int16_t accel_y = (raw_data[2] << 8) | raw_data[3];
    int16_t accel_z = (raw_data[4] << 8) | raw_data[5];

    data[0] = 0.000061 * (float)accel_x;
    data[1] = 0.000061 * (float)accel_y;
    data[2] = 0.000061 * (float)accel_z;
}

static const int16_t MAX_OFFSET_X = (X_MAX / 2) - 1;
static const int16_t MAX_OFFSET_Y = (Y_MAX / 2) - 1;

void draw_accel_lines(float ax, float ay)
{
    // center of screen
    const int16_t cx = X_MAX / 2;
    const int16_t cy = Y_MAX / 2;

    if (ax > 1.0f)
        ax = 1.0f;
    if (ax < -1.0f)
        ax = -1.0f;
    if (ay > 1.0f)
        ay = 1.0f;
    if (ay < -1.0f)
        ay = -1.0f;

    int16_t dx = (int16_t)roundf(ax * (float)MAX_OFFSET_X);
    int16_t dy = (int16_t)roundf(ay * (float)MAX_OFFSET_Y);

    if (dx == 0 && dy == 0)
    {
        ssd1306_drawPixel(cx, cy, 1);
        return;
    }

    if (dx > 0)
    {
        for (int16_t i = 0; i <= dx; i++)
        {
            ssd1306_drawPixel(cx + i, cy, 1);
        }
    }
    else if (dx < 0)
    {
        for (int16_t i = 0; i >= dx; i--)
        {
            ssd1306_drawPixel(cx + i, cy, 1);
        }
    }

    if (dy > 0)
    {
        for (int16_t j = 0; j <= dy; j++)
        {
            ssd1306_drawPixel(cx, cy + j, 1);
        }
    }
    else if (dy < 0)
    {
        for (int16_t j = 0; j >= dy; j--)
        {
            ssd1306_drawPixel(cx, cy + j, 1);
        }
    }
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
    ssd1306_clear();

    uint8_t buf[2] = {PWR_MGMT_1, 0x00};
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
    uint8_t buf1[2] = {ACCEL_CONFIG, 0x00};
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf1, 2, false);
    uint8_t buf2[2] = {GYRO_CONFIG, 0x03};
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf2, 2, false);

    while (true)
    {
        heartbeat();
        ssd1306_clear();

        float accel_data[3];
        get_mpu_data(accel_data);
        float ax = accel_data[0];
        float ay = accel_data[1];

        draw_accel_lines(ax, ay);

        ssd1306_update();
    }
}

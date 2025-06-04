/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h> // for sinf(), cosf()

#include "hardware/gpio.h"
#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"

#define UP_PIN 16
#define LEFT_PIN 2
#define DOWN_PIN 13
#define RIGHT_PIN 20
#define MODE_PIN 15
#define LED_PIN 14

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

#define SPEED_THRESHOLD_1 1000
#define SPEED_THRESHOLD_2 2000
#define SPEED_THRESHOLD_3 3000

#define SPEED_LEVEL_1 1
#define SPEED_LEVEL_2 3
#define SPEED_LEVEL_3 5
#define SPEED_LEVEL_4 8

#define CIRCLE_RADIUS 5.0f
#define ANGLE_INCREMENT 0.05f
#define PI 3.14159265358979323846f

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);

void init_gpios()
{
  uint8_t button_pins[4] = {UP_PIN, LEFT_PIN, DOWN_PIN, RIGHT_PIN};
  for (int i = 0; i < 4; i++)
  {
    gpio_init(button_pins[i]);
    gpio_set_dir(button_pins[i], GPIO_IN);
    gpio_pull_up(button_pins[i]);
  }

  gpio_init(MODE_PIN);
  gpio_set_dir(MODE_PIN, GPIO_IN);
  gpio_pull_up(MODE_PIN);

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, 0);
}

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb)
  {
    board_init_after_tusb();
  }

  init_gpios();

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();
    hid_task();
  }

  return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, uint32_t btn)
{
  // skip if hid is not ready yet
  if (!tud_hid_ready())
    return;

  switch (report_id)
  {
  case REPORT_ID_KEYBOARD:
  {
    // original keyboard handling (unchanged)
    static bool has_keyboard_key = false;

    if (btn)
    {
      uint8_t keycode[6] = {0};
      keycode[0] = HID_KEY_A;
      tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
      has_keyboard_key = true;
    }
    else
    {
      if (has_keyboard_key)
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
      has_keyboard_key = false;
    }
  }
  break;

  case REPORT_ID_MOUSE:
  {
    static bool remote_mode = false;
    static bool prev_mode_pressed = false;

    bool mode_pressed = (gpio_get(MODE_PIN) == 0);
    if (mode_pressed && !prev_mode_pressed)
    {
      remote_mode = !remote_mode;
      // LED on = remote mode, LED off = regular mode
      gpio_put(LED_PIN, remote_mode ? 1 : 0);
    }
    prev_mode_pressed = mode_pressed;

    if (remote_mode)
    {
      // move cursor in a slow circle
      static float angle = 0.0f;
      int8_t dx = (int8_t)(CIRCLE_RADIUS * cosf(angle));
      int8_t dy = (int8_t)(CIRCLE_RADIUS * sinf(angle));
      angle += ANGLE_INCREMENT;
      if (angle > 2 * PI)
        angle -= 2 * PI;
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, dx, dy, 0, 0);
    }
    else
    {
      // control with UP, DOWN, LEFT, RIGHT pins
      static uint32_t up_start = 0;
      static uint32_t down_start = 0;
      static uint32_t left_start = 0;
      static uint32_t right_start = 0;

      static bool up_prev = false;
      static bool down_prev = false;
      static bool left_prev = false;
      static bool right_prev = false;

      int8_t dx = 0;
      int8_t dy = 0;

      // UP
      bool up_pressed = (gpio_get(UP_PIN) == 0);
      if (up_pressed)
      {
        if (!up_prev)
          up_start = board_millis();
        uint32_t elapsed = board_millis() - up_start;
        uint8_t speed = (elapsed < SPEED_THRESHOLD_1)   ? SPEED_LEVEL_1
                        : (elapsed < SPEED_THRESHOLD_2) ? SPEED_LEVEL_2
                        : (elapsed < SPEED_THRESHOLD_3) ? SPEED_LEVEL_3
                                                        : SPEED_LEVEL_4;
        dy -= speed; // negative Y moves cursor up
        up_prev = true;
      }
      else
      {
        up_prev = false;
      }

      // DOWN
      bool down_pressed = (gpio_get(DOWN_PIN) == 0);
      if (down_pressed)
      {
        if (!down_prev)
          down_start = board_millis();
        uint32_t elapsed = board_millis() - down_start;
        uint8_t speed = (elapsed < SPEED_THRESHOLD_1)   ? SPEED_LEVEL_1
                        : (elapsed < SPEED_THRESHOLD_2) ? SPEED_LEVEL_2
                        : (elapsed < SPEED_THRESHOLD_3) ? SPEED_LEVEL_3
                                                        : SPEED_LEVEL_4;
        dy += speed; // positive Y moves cursor down
        down_prev = true;
      }
      else
      {
        down_prev = false;
      }

      // LEFT
      bool left_pressed = (gpio_get(LEFT_PIN) == 0);
      if (left_pressed)
      {
        if (!left_prev)
          left_start = board_millis();
        uint32_t elapsed = board_millis() - left_start;
        uint8_t speed = (elapsed < SPEED_THRESHOLD_1)   ? SPEED_LEVEL_1
                        : (elapsed < SPEED_THRESHOLD_2) ? SPEED_LEVEL_2
                        : (elapsed < SPEED_THRESHOLD_3) ? SPEED_LEVEL_3
                                                        : SPEED_LEVEL_4;
        dx -= speed; // negative X moves cursor left
        left_prev = true;
      }
      else
      {
        left_prev = false;
      }

      // RIGHT
      bool right_pressed = (gpio_get(RIGHT_PIN) == 0);
      if (right_pressed)
      {
        if (!right_prev)
          right_start = board_millis();
        uint32_t elapsed = board_millis() - right_start;
        uint8_t speed = (elapsed < SPEED_THRESHOLD_1)   ? SPEED_LEVEL_1
                        : (elapsed < SPEED_THRESHOLD_2) ? SPEED_LEVEL_2
                        : (elapsed < SPEED_THRESHOLD_3) ? SPEED_LEVEL_3
                                                        : SPEED_LEVEL_4;
        dx += speed; // positive X moves cursor right
        right_prev = true;
      }
      else
      {
        right_prev = false;
      }

      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, dx, dy, 0, 0);
    }
  }
  break;

  case REPORT_ID_CONSUMER_CONTROL:
  {
    // original consumer control handling (unchanged)
    static bool has_consumer_key = false;
    if (btn)
    {
      uint16_t volume_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
      tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
      has_consumer_key = true;
    }
    else
    {
      uint16_t empty_key = 0;
      if (has_consumer_key)
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
      has_consumer_key = false;
    }
  }
  break;

  case REPORT_ID_GAMEPAD:
  {
    // original gamepad handling (unchanged)
    static bool has_gamepad_key = false;
    hid_gamepad_report_t report = {
        .x = 0, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0, .hat = 0, .buttons = 0};

    if (btn)
    {
      report.hat = GAMEPAD_HAT_UP;
      report.buttons = GAMEPAD_BUTTON_A;
      tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
      has_gamepad_key = true;
    }
    else
    {
      report.hat = GAMEPAD_HAT_CENTERED;
      report.buttons = 0;
      if (has_gamepad_key)
        tud_hid_report(REPORT_ID_GAMEPAD, &report, sizeof(report));
      has_gamepad_key = false;
    }
  }
  break;

  default:
    break;
  }
}

// Every 10ms, we will send 1 report for each HID profile (keyboard, mouse, etc.)
// tud_hid_report_complete_cb() is used to send the next report after the previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms)
    return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  // Remote wakeup
  if (tud_suspended() && btn)
  {
    // Wake up host if we are in suspend mode and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }
  else
  {
    // Send the mouse report (REPORT_ID_MOUSE); other report IDs will be chained automatically
    send_hid_report(REPORT_ID_MOUSE, btn);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
  (void)instance;
  (void)len;

  uint8_t next_report_id = report[0] + 1u;
  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, board_button_read());
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t reqlen)
{
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint (Report ID = 0, Type = 0)
void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize)
{
  (void)instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == REPORT_ID_KEYBOARD)
  {
    if (bufsize < 1)
      return;
    uint8_t const kbd_leds = buffer[0];

    if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
    {
      // Capslock On: disable blink, turn board LED on
      blink_interval_ms = 0;
      board_led_write(true);
    }
    else
    {
      // Capslock Off: restore normal blink
      board_led_write(false);
      blink_interval_ms = BLINK_MOUNTED;
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms)
    return;

  // Blink every interval_ms
  if (board_millis() - start_ms < blink_interval_ms)
    return;
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = !led_state;
}

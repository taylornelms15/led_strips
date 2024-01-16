#ifndef LED_CONTROL_H_
#define LED_CONTROL_H_

#include <linux/printk.h>

#include "led_strip.h"

/**
 * prepare_output_gpio() - Prepares led strip to output
 * @ctx: Reference to led strip device
 *
 * Return: 0 on success, negative value on error
 */
int prepare_output_gpio(struct led_strip_priv *ctx);

/**
 * release_output_gpio() - Releases gpio resources for output
 * @ctx: Reference to led strip device
 *
 * Return: 0 on success, negative value on error
 */
int release_output_gpio(struct led_strip_priv *ctx);

/**
 * output_led_values() - Write led values to output gpio
 * @ctx: Reference to led strip device, including data to write
 * @num_leds: How many of the led values to write out
 *
 * Return: 0 on success, negative value on error
 */
int output_led_values(struct led_strip_priv *ctx, int num_leds);

#endif

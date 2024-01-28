#ifndef LED_STRIP_H_
#define LED_STRIP_H_

#include <linux/cdev.h>

#include "led_register_map.h"

#define MAX_LEDS_PER_DEVICE 300
#define MAX_LED_ARRAY_SIZE (3 * MAX_LEDS_PER_DEVICE)

struct led_registers;

enum led_strip_number {
	LED_0 = 0,
	LED_1,
};


/**
 * struct led_strip_priv: Private data for led_strip driver
 *
 * Specifically, this gets allocated on file open and freed on file close
 */
struct led_strip_priv {
	/**
	 * @dev: reference to device node
	 */
	struct device *dev;
	/**
	 * @cdev: reference to parent character device
	 */
	struct cdev *cdev;
	/**
	 * @strip_no: Which strip this is (corresponds to device minor no)
	 */
	enum led_strip_number strip_no;
	/**
	 * @reg: memory-mapped register control
	 */
	struct led_registers reg;
	/**
	 * @values_to_write: parsed values to write out to led strip 
	 * Note that there is no locking read-write access to this (currently)
	 */
	u8 values_to_write[MAX_LED_ARRAY_SIZE];
};

#endif

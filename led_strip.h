#ifndef LED_STRIP_H_
#define LED_STRIP_H_

#include <linux/cdev.h>

#define MAX_LEDS_PER_DEVICE 300
#define MAX_LED_ARRAY_SIZE (3 * MAX_LEDS_PER_DEVICE)

/**
 * struct led_strip_priv: Private data for led_strip driver
 *
 * Specifically, this gets allocated on file open and freed on file close
 */
struct led_strip_priv {
	/**
	 * @cdev: reference to parent character device
	 */
	struct cdev *cdev;
	/**
	 * @strip_no: Which strip this is (corresponds to device minor no)
	 */
	int strip_no;
	/**
	 * @values_to_write: parsed values to write out to led strip 
	 * Note that there is no locking read-write access to this (currently)
	 */
	u8 values_to_write[MAX_LED_ARRAY_SIZE];
};

#endif

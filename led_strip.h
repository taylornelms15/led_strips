#ifndef LED_STRIP_H_
#define LED_STRIP_H_

#include <linux/cdev.h>

#include "led_register_map.h"

#define MAX_LEDS_PER_DEVICE 300
#define MAX_LED_ARRAY_SIZE (3 * MAX_LEDS_PER_DEVICE)
#define SERIALIZED_PWM_LED_BITS (MAX_LED_ARRAY_SIZE * 3)
#define SERIALIZED_PWM_RESET_BITS 32 //Currently: just set low for 32 bits, then do a normal wait operation for rest of frame done
#define SERIALIZED_PWM_BITS (SERIALIZED_PWM_LED_BITS + SERIALIZED_PWM_RESET_BITS)

struct led_registers;

enum led_strip_number {
	LED_0 = 0,
	LED_1,
};


struct led_dma_region {
	struct dma_cb_t dma_cb;
	u8 data[SERIALIZED_PWM_BITS];
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
	/**
	 * @dma_mem: allocated dma memory to use for data transfer
	 */
	struct led_dma_region *dma_mem;
	/**
	 * @dma_handle: dma handle for our allocated dma_mem
	 */
	dma_addr_t dma_handle;
};

#endif

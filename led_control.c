#include <asm/io.h>
#include <linux/delay.h>
#include <linux/dma-direct.h>
#include <linux/dma-mapping.h>

#include "led_control.h"

/* Registers */
/* doc says 0x7e, which throws error; what makes it 0xfe?  Is this right?*/
#define PWM0_BASE 0xfe20c000
#define PWM1_BASE 0xfe20c800
#define CM_CLK_CTL_BASE 0xfe1010a0

#define STRIP0_DMANUM_DEFAULT 10
#define STRIP1_DMANUM_DEFAULT 11

#define USING_IO_MEMCPY

/* Notes */
#define hightime_zero "220-380ns"
#define hitime_one "580-1600ns"
#define lotime_zero "580-1600ns"
#define lotime_one "220-420ns"
#define frame_unit "280000ns"
#define frame_blank_pulses 700 //280 us / pulse_ns, plus some overflow
#define pulse_ns "407.407ns"

#define FRAME_WAIT_US 280

#define OSC_FREQ 54000000
#define TARGET_FREQ 2400000

/**
 * bit_convert_table - Table used to convert 8-bit value to 24-bit PWM signal
 *
 * Each bit of data to send will be sent as either "0b100" or "0b110"
 * for values of 0 and 1 respectively. This table allows us to find the value
 * of converted byte X by reading bit_convert_table[3*X : 3*X + 2]
 *
 * For example, the value of "0x02" is found at indices 6-8,
 * which contain "0x92, 0x49, 0x34" (0b100 100 100 100 100 100 110 100)
 * which corresponds to the value 0x02 (0b00000010)
 */
static const u8 bit_convert_table[];

/* Register Mapping */

/**
 * map_registers() - call ioremap on registers that we need to access
 */
static int map_registers(struct led_registers *reg, enum led_strip_number strip_no)
{
	int ret = 0;
	uint32_t dma_addr;

	switch(strip_no) {
	case LED_0:
		dma_addr = dmanum_to_addr(STRIP0_DMANUM_DEFAULT);
		reg->pwm = ioremap(PWM0_BASE, sizeof(struct pwm_t));
		if (!reg->pwm) {
			ret = -EIO;
			break;
		}
		break;
	case LED_1:
		dma_addr = dmanum_to_addr(STRIP1_DMANUM_DEFAULT);
		dma_addr = dmanum_to_addr(10);
		reg->pwm = ioremap(PWM1_BASE, sizeof(struct pwm_t));
		if (!reg->pwm) {
			ret = -EIO;
			break;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	if (ret)
		goto map_registers_out;

	reg->cm_clk = ioremap(CM_CLK_CTL_BASE, sizeof(struct cm_clk_t));
	if (!reg->cm_clk) {
		ret = -EIO;
		goto unmap_pwm;
	}

	reg->dma = ioremap(dma_addr, sizeof(struct dma_t));
	if (!reg->dma) {
		ret = -EIO;
		goto unmap_cm_clk;
	}

	return ret;

unmap_cm_clk:
	iounmap(reg->cm_clk);
	reg->cm_clk = NULL;
unmap_pwm:
	iounmap(reg->pwm);
	reg->pwm = NULL;
map_registers_out:
	return ret;
}

static int unmap_registers(struct led_registers *reg)
{
	iounmap(reg->dma);
	iounmap(reg->cm_clk);
	iounmap(reg->pwm);
	reg->pwm = NULL;
	return 0;
}

/* Setup */

static int setup_clocks(struct led_strip_priv *ctx)
{
	struct cm_clk_t *cm_clk = ctx->reg.cm_clk;

	/* ends up with 2.454 MHz clock, so 407.407ns per tick */
	cm_clk->div = CM_CLK_DIV_PASSWD | CM_CLK_DIV_DIVI(OSC_FREQ / TARGET_FREQ);
	cm_clk->ctl = CM_CLK_CTL_PASSWD | CM_CLK_CTL_SRC_OSC;
	cm_clk->ctl = CM_CLK_CTL_PASSWD | CM_CLK_CTL_SRC_OSC | CM_CLK_CTL_ENAB;
	usleep_range(10, 20);
	while (!(cm_clk->ctl & CM_CLK_CTL_BUSY)) {
		dev_info(ctx->dev, "waiting for clocks to set while busy\n");
		usleep_range(10, 20);
	}

	dev_info(ctx->dev, "Successfully set up clocks\n");

	return 0;
}

static int setup_dma(struct led_strip_priv *ctx)
{
	uint32_t pwm_base_addr;

	if (ctx->strip_no == LED_0)
		pwm_base_addr = PWM0_BASE;
	else
		pwm_base_addr = PWM1_BASE;

	// Alloc our transfer areas
	dma_set_coherent_mask(ctx->dev, 0xffffffff);
	ctx->dma_mem = dma_alloc_coherent(ctx->dev, sizeof(struct led_dma_region), &ctx->dma_handle, GFP_KERNEL);
	if (!ctx->dma_mem) {
		dev_err(ctx->dev, "Error allocating dma_mem\n");
		return -ENOMEM;
	}
	else {
		dev_info(ctx->dev, "Allocated dma_mem, cpu addr %p, dma_handle %#016llx, phys_addr_t %#016llx\n",
			ctx->dma_mem, ctx->dma_handle, dma_to_phys(ctx->dev, ctx->dma_handle));
	}

	// Pre-fill constant data into CB section of prealloc
	ctx->dma_mem->dma_cb.ti = RPI_DMA_TI_NO_WIDE_BURSTS |  // 32-bit transfers
				  RPI_DMA_TI_WAIT_RESP |       // wait for write complete
				  RPI_DMA_TI_DEST_DREQ |       // user peripheral flow control
				  RPI_DMA_TI_PERMAP(5) |       // PWM peripheral
				  RPI_DMA_TI_SRC_INC;          // Increment src addr
	ctx->dma_mem->dma_cb.txfr_len = SERIALIZED_PWM_BITS;
	/* pointer-math, getting physical address of the data[] array */
	ctx->dma_mem->dma_cb.source_ad = (uint32_t)(dma_to_phys(ctx->dev, ctx->dma_handle) +
		(uint32_t)((void *)&ctx->dma_mem->data - (void *)&ctx->dma_mem));
	ctx->dma_mem->dma_cb.dest_ad = pwm_base_addr + RPI_PWM_FIF1_OFFSET;
	ctx->dma_mem->dma_cb.stride = 0;
	ctx->dma_mem->dma_cb.next_cb = 0;

	// Pre-fill constant data into DMA registers

	return 0;
}

/* Teardown */

static int teardown_clocks(struct led_strip_priv *ctx)
{
	/* Nothing to tear down */
	return 0;
}

static int teardown_dma(struct led_strip_priv *ctx)
{
	if (ctx->dma_mem) {
		dma_free_coherent(ctx->dev, sizeof(struct led_dma_region), ctx->dma_mem, ctx->dma_handle);
		ctx->dma_mem = NULL;
	}

	return 0;
};

/* Transfer Functions */

/**
 * convert_brightness_values() - Converts buffer of brightness values into PWM levels
 * @out_vals: output array of memory into which to put PWM vals
 * @brightnesses: input array of brightness values
 * @num_brightnesses: how many brightnesses to write
 *
 * Note that out_vals needs to be at least three times the length of num_brightnesses
 *
 * Return: number of bytes written to out_vals, or a negative value on error
 */
int convert_brightness_values(u8 *out_vals, const u8 *brightnesses, int num_brightnesses)
{
	int i;

	for(i = 0; i < num_brightnesses; ++i) {
#ifdef USING_IO_MEMCPY
		memcpy_toio((volatile void *) &out_vals[3 * i],
			  &bit_convert_table[3 * brightnesses[i]], 3);
#else
		memcpy(&out_vals[3 * i],
		       &bit_convert_table[3 * brightnesses[i]], 3);
#endif
	}
	
	return num_brightnesses * 3;
}

/* High-Level Functions */

int prepare_output_gpio(struct led_strip_priv *ctx)
{
	int ret;
	
	ret = map_registers(&ctx->reg, ctx->strip_no);
	if (ret)
		return ret;
	ret = setup_clocks(ctx);
	if (ret)
		return ret;
	ret = setup_dma(ctx);
	if (ret)
		return ret;

	return ret;
}

int release_output_gpio(struct led_strip_priv *ctx)
{
	int ret = 0;

	ret = teardown_dma(ctx);
	if (ret)
		dev_err(ctx->dev, "%s Error tearing down dma (%d)\n", __func__, ret);
	ret = teardown_clocks(ctx);
	if (ret)
		dev_err(ctx->dev, "%s Error tearing down clocks (%d)\n", __func__, ret);
	ret = unmap_registers(&ctx->reg);
	if (ret)
		dev_err(ctx->dev, "%s Error unmapping registers (%d)\n", __func__, ret);

	return ret;
}

int output_led_values(struct led_strip_priv *ctx, int num_leds)
{
	u8 *vals = ctx->values_to_write;

	(void)vals;
	return 0;
}

/* Data */

static const u8 bit_convert_table[] = {
	0x92, 0x49, 0x24, 0x92, 0x49, 0x26, 0x92, 0x49, 0x34, 0x92, 0x49, 0x36,
	0x92, 0x49, 0xa4, 0x92, 0x49, 0xa6, 0x92, 0x49, 0xb4, 0x92, 0x49, 0xb6,
	0x92, 0x4d, 0x24, 0x92, 0x4d, 0x26, 0x92, 0x4d, 0x34, 0x92, 0x4d, 0x36,
	0x92, 0x4d, 0xa4, 0x92, 0x4d, 0xa6, 0x92, 0x4d, 0xb4, 0x92, 0x4d, 0xb6,
	0x92, 0x69, 0x24, 0x92, 0x69, 0x26, 0x92, 0x69, 0x34, 0x92, 0x69, 0x36,
	0x92, 0x69, 0xa4, 0x92, 0x69, 0xa6, 0x92, 0x69, 0xb4, 0x92, 0x69, 0xb6,
	0x92, 0x6d, 0x24, 0x92, 0x6d, 0x26, 0x92, 0x6d, 0x34, 0x92, 0x6d, 0x36,
	0x92, 0x6d, 0xa4, 0x92, 0x6d, 0xa6, 0x92, 0x6d, 0xb4, 0x92, 0x6d, 0xb6,
	0x93, 0x49, 0x24, 0x93, 0x49, 0x26, 0x93, 0x49, 0x34, 0x93, 0x49, 0x36,
	0x93, 0x49, 0xa4, 0x93, 0x49, 0xa6, 0x93, 0x49, 0xb4, 0x93, 0x49, 0xb6,
	0x93, 0x4d, 0x24, 0x93, 0x4d, 0x26, 0x93, 0x4d, 0x34, 0x93, 0x4d, 0x36,
	0x93, 0x4d, 0xa4, 0x93, 0x4d, 0xa6, 0x93, 0x4d, 0xb4, 0x93, 0x4d, 0xb6,
	0x93, 0x69, 0x24, 0x93, 0x69, 0x26, 0x93, 0x69, 0x34, 0x93, 0x69, 0x36,
	0x93, 0x69, 0xa4, 0x93, 0x69, 0xa6, 0x93, 0x69, 0xb4, 0x93, 0x69, 0xb6,
	0x93, 0x6d, 0x24, 0x93, 0x6d, 0x26, 0x93, 0x6d, 0x34, 0x93, 0x6d, 0x36,
	0x93, 0x6d, 0xa4, 0x93, 0x6d, 0xa6, 0x93, 0x6d, 0xb4, 0x93, 0x6d, 0xb6,
	0x9a, 0x49, 0x24, 0x9a, 0x49, 0x26, 0x9a, 0x49, 0x34, 0x9a, 0x49, 0x36,
	0x9a, 0x49, 0xa4, 0x9a, 0x49, 0xa6, 0x9a, 0x49, 0xb4, 0x9a, 0x49, 0xb6,
	0x9a, 0x4d, 0x24, 0x9a, 0x4d, 0x26, 0x9a, 0x4d, 0x34, 0x9a, 0x4d, 0x36,
	0x9a, 0x4d, 0xa4, 0x9a, 0x4d, 0xa6, 0x9a, 0x4d, 0xb4, 0x9a, 0x4d, 0xb6,
	0x9a, 0x69, 0x24, 0x9a, 0x69, 0x26, 0x9a, 0x69, 0x34, 0x9a, 0x69, 0x36,
	0x9a, 0x69, 0xa4, 0x9a, 0x69, 0xa6, 0x9a, 0x69, 0xb4, 0x9a, 0x69, 0xb6,
	0x9a, 0x6d, 0x24, 0x9a, 0x6d, 0x26, 0x9a, 0x6d, 0x34, 0x9a, 0x6d, 0x36,
	0x9a, 0x6d, 0xa4, 0x9a, 0x6d, 0xa6, 0x9a, 0x6d, 0xb4, 0x9a, 0x6d, 0xb6,
	0x9b, 0x49, 0x24, 0x9b, 0x49, 0x26, 0x9b, 0x49, 0x34, 0x9b, 0x49, 0x36,
	0x9b, 0x49, 0xa4, 0x9b, 0x49, 0xa6, 0x9b, 0x49, 0xb4, 0x9b, 0x49, 0xb6,
	0x9b, 0x4d, 0x24, 0x9b, 0x4d, 0x26, 0x9b, 0x4d, 0x34, 0x9b, 0x4d, 0x36,
	0x9b, 0x4d, 0xa4, 0x9b, 0x4d, 0xa6, 0x9b, 0x4d, 0xb4, 0x9b, 0x4d, 0xb6,
	0x9b, 0x69, 0x24, 0x9b, 0x69, 0x26, 0x9b, 0x69, 0x34, 0x9b, 0x69, 0x36,
	0x9b, 0x69, 0xa4, 0x9b, 0x69, 0xa6, 0x9b, 0x69, 0xb4, 0x9b, 0x69, 0xb6,
	0x9b, 0x6d, 0x24, 0x9b, 0x6d, 0x26, 0x9b, 0x6d, 0x34, 0x9b, 0x6d, 0x36,
	0x9b, 0x6d, 0xa4, 0x9b, 0x6d, 0xa6, 0x9b, 0x6d, 0xb4, 0x9b, 0x6d, 0xb6,
	0xd2, 0x49, 0x24, 0xd2, 0x49, 0x26, 0xd2, 0x49, 0x34, 0xd2, 0x49, 0x36,
	0xd2, 0x49, 0xa4, 0xd2, 0x49, 0xa6, 0xd2, 0x49, 0xb4, 0xd2, 0x49, 0xb6,
	0xd2, 0x4d, 0x24, 0xd2, 0x4d, 0x26, 0xd2, 0x4d, 0x34, 0xd2, 0x4d, 0x36,
	0xd2, 0x4d, 0xa4, 0xd2, 0x4d, 0xa6, 0xd2, 0x4d, 0xb4, 0xd2, 0x4d, 0xb6,
	0xd2, 0x69, 0x24, 0xd2, 0x69, 0x26, 0xd2, 0x69, 0x34, 0xd2, 0x69, 0x36,
	0xd2, 0x69, 0xa4, 0xd2, 0x69, 0xa6, 0xd2, 0x69, 0xb4, 0xd2, 0x69, 0xb6,
	0xd2, 0x6d, 0x24, 0xd2, 0x6d, 0x26, 0xd2, 0x6d, 0x34, 0xd2, 0x6d, 0x36,
	0xd2, 0x6d, 0xa4, 0xd2, 0x6d, 0xa6, 0xd2, 0x6d, 0xb4, 0xd2, 0x6d, 0xb6,
	0xd3, 0x49, 0x24, 0xd3, 0x49, 0x26, 0xd3, 0x49, 0x34, 0xd3, 0x49, 0x36,
	0xd3, 0x49, 0xa4, 0xd3, 0x49, 0xa6, 0xd3, 0x49, 0xb4, 0xd3, 0x49, 0xb6,
	0xd3, 0x4d, 0x24, 0xd3, 0x4d, 0x26, 0xd3, 0x4d, 0x34, 0xd3, 0x4d, 0x36,
	0xd3, 0x4d, 0xa4, 0xd3, 0x4d, 0xa6, 0xd3, 0x4d, 0xb4, 0xd3, 0x4d, 0xb6,
	0xd3, 0x69, 0x24, 0xd3, 0x69, 0x26, 0xd3, 0x69, 0x34, 0xd3, 0x69, 0x36,
	0xd3, 0x69, 0xa4, 0xd3, 0x69, 0xa6, 0xd3, 0x69, 0xb4, 0xd3, 0x69, 0xb6,
	0xd3, 0x6d, 0x24, 0xd3, 0x6d, 0x26, 0xd3, 0x6d, 0x34, 0xd3, 0x6d, 0x36,
	0xd3, 0x6d, 0xa4, 0xd3, 0x6d, 0xa6, 0xd3, 0x6d, 0xb4, 0xd3, 0x6d, 0xb6,
	0xda, 0x49, 0x24, 0xda, 0x49, 0x26, 0xda, 0x49, 0x34, 0xda, 0x49, 0x36,
	0xda, 0x49, 0xa4, 0xda, 0x49, 0xa6, 0xda, 0x49, 0xb4, 0xda, 0x49, 0xb6,
	0xda, 0x4d, 0x24, 0xda, 0x4d, 0x26, 0xda, 0x4d, 0x34, 0xda, 0x4d, 0x36,
	0xda, 0x4d, 0xa4, 0xda, 0x4d, 0xa6, 0xda, 0x4d, 0xb4, 0xda, 0x4d, 0xb6,
	0xda, 0x69, 0x24, 0xda, 0x69, 0x26, 0xda, 0x69, 0x34, 0xda, 0x69, 0x36,
	0xda, 0x69, 0xa4, 0xda, 0x69, 0xa6, 0xda, 0x69, 0xb4, 0xda, 0x69, 0xb6,
	0xda, 0x6d, 0x24, 0xda, 0x6d, 0x26, 0xda, 0x6d, 0x34, 0xda, 0x6d, 0x36,
	0xda, 0x6d, 0xa4, 0xda, 0x6d, 0xa6, 0xda, 0x6d, 0xb4, 0xda, 0x6d, 0xb6,
	0xdb, 0x49, 0x24, 0xdb, 0x49, 0x26, 0xdb, 0x49, 0x34, 0xdb, 0x49, 0x36,
	0xdb, 0x49, 0xa4, 0xdb, 0x49, 0xa6, 0xdb, 0x49, 0xb4, 0xdb, 0x49, 0xb6,
	0xdb, 0x4d, 0x24, 0xdb, 0x4d, 0x26, 0xdb, 0x4d, 0x34, 0xdb, 0x4d, 0x36,
	0xdb, 0x4d, 0xa4, 0xdb, 0x4d, 0xa6, 0xdb, 0x4d, 0xb4, 0xdb, 0x4d, 0xb6,
	0xdb, 0x69, 0x24, 0xdb, 0x69, 0x26, 0xdb, 0x69, 0x34, 0xdb, 0x69, 0x36,
	0xdb, 0x69, 0xa4, 0xdb, 0x69, 0xa6, 0xdb, 0x69, 0xb4, 0xdb, 0x69, 0xb6,
	0xdb, 0x6d, 0x24, 0xdb, 0x6d, 0x26, 0xdb, 0x6d, 0x34, 0xdb, 0x6d, 0x36,
	0xdb, 0x6d, 0xa4, 0xdb, 0x6d, 0xa6, 0xdb, 0x6d, 0xb4, 0xdb, 0x6d, 0xb6,
};

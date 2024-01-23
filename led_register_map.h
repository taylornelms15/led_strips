#ifndef LED_REGISTER_MAP_H_
#define LED_REGISTER_MAP_H_

#include <linux/types.h>

/**
 * struct pwm_t - Registers for PWM control
 */
struct __attribute__((packed, aligned(4))) pwm_t
{
	u32 ctl;
#define RPI_PWM_CTL_MSEN2                        (1 << 15)
#define RPI_PWM_CTL_USEF2                        (1 << 13)
#define RPI_PWM_CTL_POLA2                        (1 << 12)
#define RPI_PWM_CTL_SBIT2                        (1 << 11)
#define RPI_PWM_CTL_RPTL2                        (1 << 10)
#define RPI_PWM_CTL_MODE2                        (1 << 9)
#define RPI_PWM_CTL_PWEN2                        (1 << 8)
#define RPI_PWM_CTL_MSEN1                        (1 << 7)
#define RPI_PWM_CTL_CLRF1                        (1 << 6)
#define RPI_PWM_CTL_USEF1                        (1 << 5)
#define RPI_PWM_CTL_POLA1                        (1 << 4)
#define RPI_PWM_CTL_SBIT1                        (1 << 3)
#define RPI_PWM_CTL_RPTL1                        (1 << 2)
#define RPI_PWM_CTL_MODE1                        (1 << 1)
#define RPI_PWM_CTL_PWEN1                        (1 << 0)
	u32 sta;
#define RPI_PWM_STA_STA4                         (1 << 12)
#define RPI_PWM_STA_STA3                         (1 << 11)
#define RPI_PWM_STA_STA2                         (1 << 10)
#define RPI_PWM_STA_STA1                         (1 << 9)
#define RPI_PWM_STA_BERR                         (1 << 8)
#define RPI_PWM_STA_GAP04                        (1 << 7)
#define RPI_PWM_STA_GAP03                        (1 << 6)
#define RPI_PWM_STA_GAP02                        (1 << 5)
#define RPI_PWM_STA_GAP01                        (1 << 4)
#define RPI_PWM_STA_RERR1                        (1 << 3)
#define RPI_PWM_STA_WERR1                        (1 << 2)
#define RPI_PWM_STA_EMPT1                        (1 << 1)
#define RPI_PWM_STA_FULL1                        (1 << 0)
	u32 dmac;
#define RPI_PWM_DMAC_ENAB                        (1 << 31)
#define RPI_PWM_DMAC_PANIC(val)                  ((val & 0xff) << 8)
#define RPI_PWM_DMAC_DREQ(val)                   ((val & 0xff) << 0)
	u32 resvd_0x0c;
	u32 rng1;
	u32 dat1;
	u32 fif1;
	u32 resvd_0x1c;
	u32 rng2;
	u32 dat2;
};

/**
 * struct cm_clk_t - Registers for clock control
 */
struct __attribute__((packed, aligned(4))) cm_clk_t{
	u32 ctl;
#define CM_CLK_CTL_PASSWD                        (0x5a << 24)
#define CM_CLK_CTL_MASH(val)                     ((val & 0x3) << 9)
#define CM_CLK_CTL_FLIP                          (1 << 8)
#define CM_CLK_CTL_BUSY                          (1 << 7)
#define CM_CLK_CTL_KILL                          (1 << 5)
#define CM_CLK_CTL_ENAB                          (1 << 4)
#define CM_CLK_CTL_SRC_GND                       (0 << 0)
#define CM_CLK_CTL_SRC_OSC                       (1 << 0)
#define CM_CLK_CTL_SRC_TSTDBG0                   (2 << 0)
#define CM_CLK_CTL_SRC_TSTDBG1                   (3 << 0)
#define CM_CLK_CTL_SRC_PLLA                      (4 << 0)
#define CM_CLK_CTL_SRC_PLLC                      (5 << 0)
#define CM_CLK_CTL_SRC_PLLD                      (6 << 0)
#define CM_CLK_CTL_SRC_HDMIAUX                   (7 << 0)
	u32 div;
#define CM_CLK_DIV_PASSWD                        (0x5a << 24)
#define CM_CLK_DIV_DIVI(val)                     ((val & 0xfff) << 12)
#define CM_CLK_DIV_DIVF(val)                     ((val & 0xfff) << 0)
};

/**
 * struct led_registers - collection of register mappings
 *
 * Singular location containing all memory-mapped registers
 * for device control. Want to be ioremap'd on open, iounmap'd on close
 */
struct led_registers {
	struct pwm_t *pwm;
	struct cm_clk_t *cm_clk;
};
#endif

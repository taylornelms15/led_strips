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
#define RPI_PWM_FIF1_OFFSET 0x18

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

/*
 * DMA Control Block in Main Memory
 *
 * Note: Must start at a 256 byte aligned address.
 *       Use corresponding register field definitions.
 */
struct __attribute__((packed, aligned(4))) dma_cb_t{
	uint32_t ti;
	uint32_t source_ad;
	uint32_t dest_ad;
	uint32_t txfr_len;
	uint32_t stride;
	uint32_t nextconbk;
	uint32_t resvd_0x18[2];
};

/*
 * DMA register set
 */
struct __attribute__((packed, aligned(4))) dma_t{
	uint32_t cs;
#define RPI_DMA_CS_RESET                         (1 << 31)
#define RPI_DMA_CS_ABORT                         (1 << 30)
#define RPI_DMA_CS_DISDEBUG                      (1 << 29)
#define RPI_DMA_CS_WAIT_OUTSTANDING_WRITES       (1 << 28)
#define RPI_DMA_CS_PANIC_PRIORITY(val)           ((val & 0xf) << 20)
#define RPI_DMA_CS_PRIORITY(val)                 ((val & 0xf) << 16)
#define RPI_DMA_CS_ERROR                         (1 << 8)
#define RPI_DMA_CS_WAITING_OUTSTANDING_WRITES    (1 << 6)
#define RPI_DMA_CS_DREQ_STOPS_DMA                (1 << 5)
#define RPI_DMA_CS_PAUSED                        (1 << 4)
#define RPI_DMA_CS_DREQ                          (1 << 3)
#define RPI_DMA_CS_INT                           (1 << 2)
#define RPI_DMA_CS_END                           (1 << 1)
#define RPI_DMA_CS_ACTIVE                        (1 << 0)
	uint32_t conblk_ad;
	uint32_t ti;
#define RPI_DMA_TI_NO_WIDE_BURSTS                (1 << 26)
#define RPI_DMA_TI_WAITS(val)                    ((val & 0x1f) << 21)
#define RPI_DMA_TI_PERMAP(val)                   ((val & 0x1f) << 16)
#define RPI_DMA_TI_BURST_LENGTH(val)             ((val & 0xf) << 12)
#define RPI_DMA_TI_SRC_IGNORE                    (1 << 11)
#define RPI_DMA_TI_SRC_DREQ                      (1 << 10)
#define RPI_DMA_TI_SRC_WIDTH                     (1 << 9)
#define RPI_DMA_TI_SRC_INC                       (1 << 8)
#define RPI_DMA_TI_DEST_IGNORE                   (1 << 7)
#define RPI_DMA_TI_DEST_DREQ                     (1 << 6)
#define RPI_DMA_TI_DEST_WIDTH                    (1 << 5)
#define RPI_DMA_TI_DEST_INC                      (1 << 4)
#define RPI_DMA_TI_WAIT_RESP                     (1 << 3)
#define RPI_DMA_TI_TDMODE                        (1 << 1)
#define RPI_DMA_TI_INTEN                         (1 << 0)
	uint32_t source_ad;
	uint32_t dest_ad;
	uint32_t txfr_len;
#define RPI_DMA_TXFR_LEN_YLENGTH(val)            ((val & 0xffff) << 16)
#define RPI_DMA_TXFR_LEN_XLENGTH(val)            ((val & 0xffff) << 0)
	uint32_t stride;
#define RPI_DMA_STRIDE_D_STRIDE(val)             ((val & 0xffff) << 16)
#define RPI_DMA_STRIDE_S_STRIDE(val)             ((val & 0xffff) << 0)
	uint32_t nextconbk;
	uint32_t debug;
};

#define DMA0_OFFSET                              (0x00007000)
#define DMA1_OFFSET                              (0x00007100)
#define DMA2_OFFSET                              (0x00007200)
#define DMA3_OFFSET                              (0x00007300)
#define DMA4_OFFSET                              (0x00007400)
#define DMA5_OFFSET                              (0x00007500)
#define DMA6_OFFSET                              (0x00007600)
#define DMA7_OFFSET                              (0x00007700)
#define DMA8_OFFSET                              (0x00007800)
#define DMA9_OFFSET                              (0x00007900)
#define DMA10_OFFSET                             (0x00007a00)
#define DMA11_OFFSET                             (0x00007b00)
#define DMA12_OFFSET                             (0x00007c00)
#define DMA13_OFFSET                             (0x00007d00)
#define DMA14_OFFSET                             (0x00007e00)
#define DMA15_OFFSET                             (0x00e05000)
static const uint32_t dma_offset[] =
{
    DMA0_OFFSET,
    DMA1_OFFSET,
    DMA2_OFFSET,
    DMA3_OFFSET,
    DMA4_OFFSET,
    DMA5_OFFSET,
    DMA6_OFFSET,
    DMA7_OFFSET,
    DMA8_OFFSET,
    DMA9_OFFSET,
    DMA10_OFFSET,
    DMA11_OFFSET,
    DMA12_OFFSET,
    DMA13_OFFSET,
    DMA14_OFFSET,
    DMA15_OFFSET,
};

#define dmanum_to_addr(dmanum) \
	(0xfe000000 + dma_offset[dmanum])

/**
 * struct led_registers - collection of register mappings
 *
 * Singular location containing all memory-mapped registers
 * for device control. Want to be ioremap'd on open, iounmap'd on close
 */
struct led_registers {
	struct pwm_t *pwm;
	struct cm_clk_t *cm_clk;
	struct dma_cb_t *dma_cb;
	struct dma_t *dma;
};
#endif

rpi-ws281x-objs = rpi_ws281x/rpihw.o
rpi-ws281x-objs += rpi_ws281x/mailbox.o
rpi-ws281x-objs += rpi_ws281x/ws2811.o
rpi-ws281x-objs += rpi_ws281x/pwm.o
rpi-ws281x-objs += rpi_ws281x/pcm.o
rpi-ws281x-objs += rpi_ws281x/dma.o

led-strip-objs = led_strip.o led_control.o

obj-m += led-strip.o #rpi-ws281x.o

GCC_DIR := $(shell ${CC} -print-search-dirs | grep install | cut -f2 -d" ")
GCC_INC_DIR := ${GCC_DIR}include
$(warning ${GCC_INC_DIR})
EXTRA_CFLAGS += -I$(PWD)/rpi_ws281x
#EXTRA_CFLAGS += -I${GCC_INC_DIR}

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

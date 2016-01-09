PWD=$(shell pwd)
LINUX_SRC=/home/toolchain/tools/linux
CCPREFIX=/home/toolchain/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf- 

obj-m +=mrf24j40-drv.o
all:
	make ARCH=arm CROSS_COMPILE=$(CCPREFIX) -C $(LINUX_SRC) SUBDIRS=$(PWD) -I$(PWD) modules
clean:
	make ARCH=arm CROSS_COMPILE=$(CCPREFIX) -C $(LINUX_SRC) SUBDIRS=$(PWD) -I$(PWD) clean

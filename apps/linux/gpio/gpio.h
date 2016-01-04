#ifndef __GPIO_H__
#define __GPIO_H__

/*
 * /dev/gpio_drv
 * open, close
 */ 
#define GPIO_IOC_EXPORT_PIN			0x01	// args = &(unsigned int)
/*
 * /dev/gpio_drv_x
 * open, close, read, write, poll
 */
#define GPIO_IOC_WR_DIR				0x03
#define GPIO_IOC_WR_INTR			0x04

#define GPIO_IOC_RD_DIR				(0x08 | GPIO_IOC_WR_DIR)
#define GPIO_IOC_RD_INTR			(0x08 | GPIO_IOC_WRGPIO_IOC_WR_INTR_DIR)

enum GPIO_DIR{
	GPIO_DIR_OUTPUT = 0,
	GPIO_DIR_INPUT,
};
enum GPIO_INTR{
	GPIO_INTR_NONE = 0,
	GPIO_INTR_RISING,
	GPIO_INTR_FALLING,
	GPIO_INTR_BOTH,
};
#endif

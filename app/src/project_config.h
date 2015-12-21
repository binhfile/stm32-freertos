/*
 * project_config.h
 *
 *  Created on: Dec 18, 2015
 *      Author: dev
 */

#ifndef PROJECT_CONFIG_H_
#define PROJECT_CONFIG_H_


#if defined(STM32F4XX)
#include <drv_api.h>
#include "../../extralib/special/include/gpio.h"
#include <spidev.h>
#include <termios.h>
#define DEV_RF_NAME			"rf"
#define DEV_RF_CS_NAME		"rf-cs"
#define DEV_RF_RESET_NAME	"rf-reset"
#define DEV_RF_INTR_NAME	"rf-intr"

#define DEV_EEPROM_CS_NAME		"at93c-cs"
#define DEV_EEPROM_SCK_NAME		"at93c-sck"
#define DEV_EEPROM_MOSI_NAME	"at93c-mosi"
#define DEV_EEPROM_MISO_NAME	"at93c-miso"
#elif defined(OS_LINUX)
#include <linux/spi/spidev.h>
#include <asm/termios.h>
#include <poll.h>
#define configMINIMAL_STACK_SIZE 0

#define DEV_RF_NAME			"/dev/spidev0.1"
#endif

/*
 * echo 23 > /sys/class/gpio/export
 * echo 24 > /sys/class/gpio/export
 * echo falling > /sys/class/gpio/gpio23/edge
 * echo out > /sys/class/gpio/gpio24/direction
 *
 */
#endif /* PROJECT_CONFIG_H_ */

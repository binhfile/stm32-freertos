#include <drv_api.h>
#include <drv_usart.h>
#include <drv_gpio.h>
#include <spidev.h>
/**
 * A0 - Button
 * A1 - RF
 * A2 -
 * A3 -
 *
 * B6 - DEBUG
 * B7 -
 * B10- RF
 *
 * C2 - RF
 * C3 -
 *
 * D12- LED green
 * D13-
 * D14-
 * D15-
 *
 * E2 - EEPROM
 * E4 -
 * E5 -
 * E6 -
 *
 */
/*usart*/
struct usart_platform_data g_usart_debug_data = {
	.tx_pin = gpio_get_pin('B', 6),// PB6
	.rx_pin = gpio_get_pin('B', 7),// PB7
};
struct platform_device g_usart_debug_device = {
	.dev_name 	= "debug-dev",
	.name     	= "usart-drv",
	.id 	  	= 0,
	.dev 		= {
		.platform_data = &g_usart_debug_data,
	},
	.next 		= 0,
};
/*gpio*/
struct gpio_platform_data g_gpio_output_data = {
	.dir 	= GPIO_OUTPUT,
	.pull 	= GPIO_PuPd_UP,
};
struct gpio_platform_data g_gpio_mrf_output_data = {
	.dir 	= GPIO_OUTPUT,
	.pull 	= GPIO_PuPd_NOPULL,
};
struct gpio_platform_data g_gpio_button_data = {
	.dir 	= GPIO_INPUT,
	.pull 	= GPIO_NOPULL,
	.intr 	= {
		.mode 		= GPIO_INTR_MODE_INTERRUPT,
		.trigger 	= GPIO_INTR_TRIGGER_FALLING,
	},
};
struct gpio_platform_data g_gpio_input_data = {
	.dir 	= GPIO_INPUT,
	.pull 	= GPIO_NOPULL,
	.intr 	= {
		.mode 		= GPIO_INTR_MODE_DISABLE,
	},
};
struct platform_device g_gpio_led_green_device = {
	.dev_name 	= "led-green",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_output_data,
	},
	.id 		= gpio_get_pin('D', 12),	// D12
	.next 		= 0,
};
struct platform_device g_gpio_led_orange_device = {
	.dev_name 	= "led-orange",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_output_data,
	},
	.id 		= gpio_get_pin('D', 13),	// D13
	.next 		= 0,
};
struct platform_device g_gpio_led_red_device = {
	.dev_name 	= "led-red",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_output_data,
	},
	.id			= gpio_get_pin('D', 14),	// D14
	.next 		= 0,
};
struct platform_device g_gpio_led_blue_device = {
	.dev_name 	= "led-blue",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_output_data,
	},
	.id 		= gpio_get_pin('D', 15),	// D15
	.next 		= 0,
};
struct platform_device g_gpio_button_device = {
	.dev_name 	= "button",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_button_data,
	},
	.id 		= gpio_get_pin('A', 0),	// A0
	.next 		= 0,
};
struct platform_device g_gpio_rf_cs_device = {
	.dev_name 	= "rf-cs",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_mrf_output_data,
	},
	.id 		= gpio_get_pin('A', 1),
	.next 		= 0,
};
struct platform_device g_gpio_rf_reset_device = {
	.dev_name 	= "rf-reset",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_mrf_output_data,
	},
	.id 		= gpio_get_pin('A', 2),
	.next 		= 0,
};
struct platform_device g_gpio_rf_intr_device = {
	.dev_name 	= "rf-intr",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_button_data,
	},
	.id 		= gpio_get_pin('A', 3),
	.next 		= 0,
};
/*spi*/
struct spi_platform_data g_rf_data = {
	.sck_pin 	= gpio_get_pin('B', 10),	// SPI_1
	.ss_pin 	= GPIO_PIN_INVALID,
	.mosi_pin 	= gpio_get_pin('C', 3),
	.miso_pin 	= gpio_get_pin('C', 2),
};
struct platform_device g_rf_device = {
	.dev_name 	= "rf",
	.name     	= "spidev-drv",
	.id 	  	= 1,
	.dev 		= {
		.platform_data = &g_rf_data,
	},
	.next 		= 0,
};
/*eeprom device*/
struct platform_device g_gpio_at93c_cs_device = {
	.dev_name 	= "at93c-cs",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_output_data,
	},
	.id 		= gpio_get_pin('E', 6),
	.next 		= 0,
};
struct platform_device g_gpio_at93c_sck_device = {
	.dev_name 	= "at93c-sck",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_output_data,
	},
	.id 		= gpio_get_pin('E', 4),
	.next 		= 0,
};
struct platform_device g_gpio_at93c_mosi_device = {
	.dev_name 	= "at93c-mosi",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_output_data,
	},
	.id 		= gpio_get_pin('E', 2),
	.next 		= 0,
};
struct platform_device g_gpio_at93c_miso_device = {
	.dev_name 	= "at93c-miso",
	.name     	= "gpio-drv",
	.dev 		= {
		.platform_data = &g_gpio_input_data,
	},
	.id 		= gpio_get_pin('E', 5),
	.next 		= 0,
};
// random
struct platform_device g_random_device = {
	.dev_name 	= "random-dev",
	.name     	= "random-drv",
	.dev 		= {
		.platform_data = 0,
	},
	.id 		= 0,
	.next 		= 0,
};
struct platform_device g_rtc_device = {
	.dev_name 	= "rtc0",
	.name     	= "rtc-drv",
	.dev 		= {
		.platform_data = 0,
	},
	.id 		= 0,
	.next 		= 0,
};

device_init(g_usart_debug_device);

device_init(g_gpio_led_red_device);
device_init(g_gpio_led_green_device);
device_init(g_gpio_led_orange_device);
device_init(g_gpio_led_blue_device);
device_init(g_gpio_button_device);
device_init(g_gpio_rf_cs_device);
device_init(g_gpio_rf_reset_device);
device_init(g_gpio_rf_intr_device);

device_init(g_gpio_at93c_cs_device);
device_init(g_gpio_at93c_sck_device);
device_init(g_gpio_at93c_mosi_device);
device_init(g_gpio_at93c_miso_device);

device_init(g_rf_device);
device_init(g_random_device);
device_init(g_rtc_device);

int board_register_devices(){
	platform_device_register(&g_usart_debug_device);
	
	platform_device_register(&g_gpio_led_red_device);
	platform_device_register(&g_gpio_led_green_device);
	platform_device_register(&g_gpio_led_orange_device);
	platform_device_register(&g_gpio_led_blue_device);
	platform_device_register(&g_gpio_button_device);
	platform_device_register(&g_gpio_rf_cs_device);
	platform_device_register(&g_gpio_rf_reset_device);
	platform_device_register(&g_gpio_rf_intr_device);
	
	platform_device_register(&g_gpio_at93c_cs_device);
	platform_device_register(&g_gpio_at93c_sck_device);
	platform_device_register(&g_gpio_at93c_mosi_device);
	platform_device_register(&g_gpio_at93c_miso_device);

	platform_device_register(&g_rf_device);
	platform_device_register(&g_random_device);
	platform_device_register(&g_rtc_device);
	return 0;
}
//end of file



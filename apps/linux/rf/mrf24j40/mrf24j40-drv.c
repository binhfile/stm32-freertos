/******************************************************************************/

/***************************** Include Files *********************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/mutex.h>

#include <linux/spi/spi.h>
#include <linux/gpio.h>

#include "../rf_mac.h"
/************************** Constant Definitions *****************************/
#define MRF24J40_DRV_NAME		"mrf24j40"
#define LREP(x, args...)  		printk(KERN_ALERT   MRF24J40_DRV_NAME ":" x , ##args)
#define LREP_WARN(x, args...)  	printk(KERN_WARNING MRF24J40_DRV_NAME ": %d@%s" x , __LINE__, __FILE__, ##args)
/**************************** Type Definitions *******************************/

struct MRF24J40_CONTEXT
{
	dev_t 				dev;
	struct cdev 		*c_dev;
	struct class 		*c_class;

	struct spi_device *spi_device;
};

/************************** Variable Definitions *****************************/
struct MRF24J40_CONTEXT	g_mrf24j40_ctx;
/***************** Macros (Inline Functions) Definitions *********************/
/*****************************************************************************/
static irqreturn_t mrf24j40_isr (int irq, void *dev_id, struct pt_regs *regs)
{
	//unsigned long flags;
	
	//local_irq_save(flags);
	
	
	
	//local_irq_restore(flags);
	return IRQ_HANDLED;
}

static int mrf24j40_open(struct inode *inode, struct file *filp)
{
	return 0;
}
static int mrf24j40_release(struct inode *inode, struct file *filp)
{
	return 0;
}
static ssize_t mrf24j40_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    return -EINVAL;;
}
static ssize_t mrf24j40_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{	
    return -EINVAL;
}
static long mrf24j40_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){

	return -EINVAL;
}
static unsigned int mrf24j40_poll(struct file *filp, struct poll_table_struct * wait)
{	
	return -EINVAL;
}
static int mrf24j40_spi_probe(struct spi_device *spi_device)
{
	g_mrf24j40_ctx.spi_device = spi_device;
	return 0;
}

static int mrf24j40_spi_remove(struct spi_device *spi_device)
{
	g_mrf24j40_ctx.spi_device = NULL;
	return 0;
}
/*File operations of device*/
const char g_mrf24j40_drv_name[] = "mrf24j40";
static struct file_operations mrf24j40_fops = {
	.owner 			= THIS_MODULE,
	.read 			= mrf24j40_read,
	.write 			= mrf24j40_write,
	.unlocked_ioctl = mrf24j40_ioctl,
	.open 			= mrf24j40_open,
	.release 		= mrf24j40_release,
	.poll 			= mrf24j40_poll
};
static struct spi_driver mrf24j40_spi_drv = {
	.driver = {
		.name =	g_mrf24j40_drv_name,
		.owner = THIS_MODULE,
	},
	.probe = mrf24j40_spi_probe,
	.remove = mrf24j40_spi_remove,
};
static int mrf24j40_init(void)
{
	int result = 0;
	struct spi_master *spi_master;
	struct spi_device *spi_device;
	char buff[64];
	struct device *pdev;
	
	g_mrf24j40_ctx.c_dev = 0;
    // mk dev
	g_mrf24j40_ctx.dev = MKDEV(0, 0);

	result = alloc_chrdev_region(&g_mrf24j40_ctx.dev, 0, 1, MRF24J40_DRV_NAME);
	if(result < 0){
		LREP_WARN("alloc_chrdev_region fail %d\r\n", result);
		return -1;
	}
	// register file operations
	g_mrf24j40_ctx.c_dev = cdev_alloc();
	cdev_init(g_mrf24j40_ctx.c_dev, &mrf24j40_fops);
	result = cdev_add(g_mrf24j40_ctx.c_dev, g_mrf24j40_ctx.dev, 1);
	if(result < 0){
		LREP_WARN("cdev_add fail %d\r\n", result);
		goto INIT_FAIL_1;
	}

	g_mrf24j40_ctx.c_class = class_create(THIS_MODULE, MRF24J40_DRV_NAME);
	if(!device_create(g_mrf24j40_ctx.c_class, NULL, MKDEV(MAJOR(g_mrf24j40_ctx.dev), 0), NULL, MRF24J40_DRV_NAME)){
		LREP_WARN("device_create fail\r\n");
		goto INIT_FAIL_2;
	}

	// init spi device
	result = spi_register_driver(&mrf24j40_spi_drv);
	if (result < 0) {
		LREP_WARN("spi_register_driver() failed %d\n", result);
		goto INIT_FAIL_2;
	}
	spi_master = spi_busnum_to_master(0);	// SPI bus 0
	if (!spi_master) {
		LREP_WARN("spi_busnum_to_master(%d) returned NULL\n", 1);
		goto INIT_FAIL_2;
	}
	spi_device = spi_alloc_device(spi_master);
	if (!spi_device) {
		put_device(&spi_master->dev);
		LREP_WARN("spi_alloc_device() failed\n");
		goto INIT_FAIL_2;
	}
	spi_device->chip_select = 0;	// SPI chip select 0

	/* Check whether this SPI bus.cs is already claimed */
	snprintf(buff, sizeof(buff), "%s.%u", dev_name(&spi_device->master->dev), spi_device->chip_select);

	pdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buff);
	if (pdev) {
		/* We are not going to use this spi_device, so free it */
		spi_dev_put(spi_device);
		/*
		 * There is already a device configured for this bus.cs
		 * It is okay if it us, otherwise complain and fail.
		 */
		if (pdev->driver && pdev->driver->name && strcmp(g_mrf24j40_drv_name, pdev->driver->name)) {
			LREP_WARN("Driver [%s] already registered for %s\n", pdev->driver->name, buff);
			goto INIT_FAIL_2;
		}
	} else {
		spi_device->max_speed_hz 	= 5000000;
		spi_device->mode 			= SPI_MODE_0;
		spi_device->bits_per_word 	= 8;
		spi_device->irq 			= -1;
		spi_device->controller_state= NULL;
		spi_device->controller_data = NULL;
		strlcpy(spi_device->modalias, g_mrf24j40_drv_name, SPI_NAME_SIZE);

		result = spi_add_device(spi_device);
		if (result < 0) {
			spi_dev_put(spi_device);
			LREP_WARN("spi_add_device() failed: %d\n", result);
			goto INIT_FAIL_3;
		}
	}
	put_device(&spi_master->dev);

	LREP("init done\r\n");
	result = 0;
	goto INIT_DONE;
INIT_FAIL_3:
	spi_unregister_driver(&mrf24j40_spi_drv);
INIT_FAIL_2:
	class_destroy(g_mrf24j40_ctx.c_class);
	cdev_del(g_mrf24j40_ctx.c_dev);
INIT_FAIL_1:
	unregister_chrdev_region(g_mrf24j40_ctx.dev, 1);
	result = -1;
INIT_DONE:
	return result;
}
static void mrf24j40_exit(void)
{
	spi_unregister_device(g_mrf24j40_ctx.spi_device);
	spi_unregister_driver(&mrf24j40_spi_drv);

	device_destroy(g_mrf24j40_ctx.c_class, g_mrf24j40_ctx.dev);
	class_destroy(g_mrf24j40_ctx.c_class);
	cdev_del(g_mrf24j40_ctx.c_dev);
	unregister_chrdev_region(g_mrf24j40_ctx.dev, 1);	
	LREP("exit\r\n");
}

module_init(mrf24j40_init);
module_exit(mrf24j40_exit);

MODULE_AUTHOR("BinhNT");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");

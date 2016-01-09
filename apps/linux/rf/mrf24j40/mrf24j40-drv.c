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
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/signal.h>
#include <asm/io.h>
#include <asm/uaccess.h>

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
	int major;	
};

/************************** Variable Definitions *****************************/
struct MRF24J40_CONTEXT	g_mrf24j40_ctx;
static struct class 	*g_mrf24j40_class;
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
/*File operations of device*/
static struct file_operations gpio_fops = {
	.owner 			= THIS_MODULE,
	.read 			= mrf24j40_read,
	.write 			= mrf24j40_write,
	.unlocked_ioctl = mrf24j40_ioctl,
	.open 			= mrf24j40_open,
	.release 		= mrf24j40_release,
	.poll 			= mrf24j40_poll
};
static int mrf24j40_init(void)
{
	int result = 0;
	
	g_mrf24j40_ctx.c_dev = 0;
    
	result = alloc_chrdev_region(&g_mrf24j40_ctx.dev, 0, 1, MRF24J40_DRV_NAME);
	if(result < 0){
		LREP_WARN("alloc_chrdev_region fail %d\r\n", result);
		return -1;
	}
	g_mrf24j40_ctx.major = MAJOR(g_mrf24j40_ctx.dev);
	
	g_mrf24j40_class = class_create(THIS_MODULE, MRF24J40_DRV_NAME);
	device_create(g_mrf24j40_class, NULL, MKDEV(g_mrf24j40_ctx.major, 0), NULL, MRF24J40_DRV_NAME);
	// register file operations
	g_mrf24j40_ctx.c_dev = cdev_alloc();
	cdev_init(g_mrf24j40_ctx.c_dev, &gpio_fops);
	result = cdev_add(g_mrf24j40_ctx.c_dev, g_mrf24j40_ctx.dev, 255);
	if(result < 0){
		LREP_WARN("cdev_add fail %d\r\n", result);
		unregister_chrdev(g_mrf24j40_ctx.major, MRF24J40_DRV_NAME);
		unregister_chrdev_region(g_mrf24j40_ctx.dev, 255);	
		return -1;
	}	
	return result;
}
static void mrf24j40_exit(void)
{
	cdev_del(g_mrf24j40_ctx.c_dev);
	unregister_chrdev(g_mrf24j40_ctx.major, MRF24J40_DRV_NAME);
	unregister_chrdev_region(g_mrf24j40_ctx.dev, 1);	
	class_destroy(g_mrf24j40_class);	
}

module_init(mrf24j40_init);
module_exit(mrf24j40_exit);

MODULE_AUTHOR("BinhNT");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");

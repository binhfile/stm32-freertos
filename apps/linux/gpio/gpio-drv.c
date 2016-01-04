/** @file   gpio-driver.c
 *  @brief 	Driver for GPIO
 * /dev/gpio_drv	Open pinX
 * /dev/gpio_drv_x	PinX driver
 ******************************************************************************/

/***************************** Include Files *********************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "gpio.h"
/************************** Constant Definitions *****************************/
#define GPIO_BASE_REG	(0x7E200000)
#define GPIO_MEM_LEN	(0xB0)
#define GPIO_DRV_NAME	"gpio_drv"
// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) 		*(g_gpio_ctx.mem + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) 		*(g_gpio_ctx.mem + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) 	*(g_gpio_ctx.mem + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET(g)			*(g_gpio_ctx.mem + 7 + ((g)/32)) |= (1 << (g)%32)
#define GPIO_CLR(g)			*(g_gpio_ctx.mem + 10 + ((g)/32)) |= (1 << (g)%32)

#define GPIO_READ(g) 		((*(g_gpio_ctx.mem + 13 + ((g)/32))) &= (1<< (g)%32))

#define GPIO_SET_INTR_RISING(g)			*(g_gpio_ctx.mem + 0x4C + ((g)/32)) |= (1 << (g)%32)
#define GPIO_SET_INTR_FALLING(g)		*(g_gpio_ctx.mem + 0x58 + ((g)/32)) |= (1 << (g)%32)
#define GPIO_SET_INTR_NONE(g)			{\
	*(g_gpio_ctx.mem + 0x4C + ((g)/32)) &= ~(1 << (g)%32);\
	*(g_gpio_ctx.mem + 0x58 + ((g)/32)) &= ~(1 << (g)%32);\
}

#define LREP(x, args...)  		printk(KERN_ALERT   GPIO_DRV_NAME ":" x , ##args)
#define LREP_WARN(x, args...)  	printk(KERN_WARNING GPIO_DRV_NAME ": %d@%s" x , __LINE__, __FILE__, ##args)
/**************************** Type Definitions *******************************/
struct GPIO_PIN_CONTEXT{
	int					pin;
	int					irq;
	wait_queue_head_t	wait_queue;
	char 				name[32];
	unsigned char		state;
	struct GPIO_PIN_CONTEXT *next;
};
struct GPIO_CONTEXT
{
	// /dev/gpio_drv
	dev_t 				dev;
	struct cdev 		*c_dev;
	struct resource 	*res;	
	unsigned int		*mem;
	int major;
	// dev/gpio_drv_x
	dev_t 				x_dev;
	struct cdev 		*x_c_dev;
	int					x_major;
	
	struct GPIO_PIN_CONTEXT	*pin;	
};

/************************** Variable Definitions *****************************/
struct GPIO_CONTEXT	g_gpio_ctx;
static struct class *g_gpio_class;
static struct class *g_gpio_class_x;
/***************** Macros (Inline Functions) Definitions *********************/
/*****************************************************************************/
irqreturn_t gpio_x_isr (int iIrq, void * dev_id)
{
	struct GPIO_PIN_CONTEXT *ctx = NULL;
	
	ctx = (struct GPIO_PIN_CONTEXT *)dev_id;
	if(!ctx) return 0;
	
	ctx->state = 1;
	wake_up(&ctx->wait_queue);
	
	return -EINVAL;
}
struct GPIO_PIN_CONTEXT* gpio_pin_to_ctx(int pin){
	struct GPIO_PIN_CONTEXT* ctx = 0;
	struct GPIO_PIN_CONTEXT* ptr;
	ptr = g_gpio_ctx.pin;
	while(ptr){
		if(ptr->pin == pin){
			ctx = ptr;
			break;
		}
		ptr = ptr->next;
	}
	return ctx;
}
int gpio_pin_set_dir(int pin, enum GPIO_DIR dir){
	INP_GPIO(pin);
	if(dir == GPIO_DIR_OUTPUT) OUT_GPIO(pin);
	return 0;
}
int gpio_pin_set_intr(int pin, enum GPIO_INTR intr){
	struct GPIO_PIN_CONTEXT* ctx;
	int ret;
	if(intr == GPIO_INTR_NONE) {
		GPIO_SET_INTR_NONE(pin);
	}
	else if(intr == GPIO_INTR_RISING) {
		GPIO_SET_INTR_RISING(pin);
	}
	else if(intr == GPIO_INTR_FALLING){
		 GPIO_SET_INTR_FALLING(pin);
	}
	else if(intr == GPIO_INTR_BOTH){
		GPIO_SET_INTR_RISING(pin);
		GPIO_SET_INTR_FALLING(pin);
	}
	ctx = gpio_pin_to_ctx(pin);
	if(ctx){
		if(ctx->irq == -1){
			ctx->irq = gpio_to_irq(pin);
			ret = request_irq(ctx->irq, gpio_x_isr, IRQF_IRQPOLL | IRQF_SHARED, ctx->name, ctx);
			if(ret) LREP_WARN("request_irq %d fail %d\r\n", ctx->irq, ret);
			else{
				init_waitqueue_head(&ctx->wait_queue);
			}
		}
	}else LREP_WARN("ctx is null\r\n");
	return 0;
}
unsigned char gpio_pin_read(int pin){
	return GPIO_READ(pin);
}
unsigned char gpio_pin_write(int pin, unsigned char val){
	if(val == 0) GPIO_CLR(pin);
	else  GPIO_SET(pin);
	return 0;
}
EXPORT_SYMBOL(gpio_pin_set_dir);
EXPORT_SYMBOL(gpio_pin_set_intr);
EXPORT_SYMBOL(gpio_pin_read);
EXPORT_SYMBOL(gpio_pin_write);

static int gpio_x_open(struct inode *inode, struct file *filp)
{
	return 0;
}
static int gpio_x_release(struct inode *inode, struct file *filp)
{
	return 0;
}
static ssize_t gpio_x_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	unsigned char ucval = 0;
	int minor = iminor(file_inode(filp));
	unsigned char *puc = 0;
	
	ucval = gpio_pin_read(minor);
	puc = (unsigned char *)buff;
	if(count < 0) count = 0;
	while(count --){
		if(copy_to_user((void __user *)puc, (const void*)&ucval, 1) == 0) break;
		puc++;
	}
    return count;
}
static ssize_t gpio_x_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	unsigned char ucval = 0;
	int minor = iminor(file_inode(filp));
	
	if(count > 0){
		if(copy_from_user((void*)&ucval, (const void __user *)buff, 1) != 0){
			gpio_pin_write(minor, ucval);
			return count;
		}
	}
	
    return -EINVAL;
}
static long gpio_x_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int minor = iminor(file_inode(filp));
	int ret = -EINVAL;
	unsigned int *pui = (unsigned int*)arg;
	switch(cmd){
		case GPIO_IOC_WR_DIR:{
			ret = gpio_pin_set_dir(minor, *pui);
			break;
		}
		case GPIO_IOC_WR_INTR:{
			ret = gpio_pin_set_intr(minor, *pui);
			break;
		}
		default: break;
	}
	return ret;
}
static unsigned int gpio_x_poll(struct file *filp, struct poll_table_struct * wait)
{
	int pin = iminor(file_inode(filp));
	unsigned int ret = 0;
	struct GPIO_PIN_CONTEXT* ctx;
	
	ctx = gpio_pin_to_ctx(pin);
	if(!ctx) return -1;
	if(ctx->irq == -1) return -1;
	
	poll_wait(filp, &ctx->wait_queue, wait);
	if(ctx->state){
		ctx->state = 0;
		ret = POLLIN|POLLRDNORM;
	}	
	return ret;
}
static long GPIOIoctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int exist;
	
	switch(cmd)
	{
		case GPIO_IOC_EXPORT_PIN:{
			unsigned int pin = *((unsigned int*)arg);	
			struct GPIO_PIN_CONTEXT* ptr=0;
			struct GPIO_PIN_CONTEXT* pin_ctx ;
			// check overflow
			if(pin >= 64){
				ret = -1;
				return ret;
			}
			// search exist
			exist = 0;
			if(g_gpio_ctx.pin){
				ptr = g_gpio_ctx.pin;
				while(ptr->next != 0){
					if(ptr->pin == pin){
						exist = 1;
						break;
					 }
					 ptr = ptr->next;
				}
			}
			if(exist){
				LREP(GPIO_DRV_NAME "_%d is exist\r\n", pin);
				break;
			}
			device_create(g_gpio_class_x, NULL, MKDEV(g_gpio_ctx.x_major, pin), NULL, GPIO_DRV_NAME "_%d", pin);
			GPIO_SET_INTR_NONE(pin);
			INP_GPIO(pin);
			pin_ctx = (struct GPIO_PIN_CONTEXT*)kmalloc(sizeof(struct GPIO_PIN_CONTEXT), GFP_ATOMIC);
			pin_ctx->next = 0;
			pin_ctx->pin = pin;
			pin_ctx->irq = -1;
			pin_ctx->state = 0;
			snprintf(pin_ctx->name, 31, GPIO_DRV_NAME "_%d", pin);
			if(g_gpio_ctx.pin){
				ptr = g_gpio_ctx.pin;
				while(ptr->next != 0) ptr = ptr->next;
				ptr->next = pin_ctx;
			}else{
				g_gpio_ctx.pin = pin_ctx;
			}
			LREP("export pin %d, set as input\r\n", pin);
			break;
		}
		default:
			LREP_WARN("unknown IOCTL cmd %d", cmd);
			ret = -EINVAL;
			break;
	}
	return ret;
}

/*File operations of device*/
static struct file_operations gpio_fops = {
	.owner 			= THIS_MODULE,
	.read 			= 0,
	.write 			= 0,
	.unlocked_ioctl = GPIOIoctl,
	.open 			= 0,
	.release 		= 0,
	.poll 			= 0,
};
static struct file_operations gpio_x_fops = {
	.owner 			= THIS_MODULE,
	.read 			= gpio_x_read,
	.write 			= gpio_x_write,
	.unlocked_ioctl = gpio_x_ioctl,
	.open 			= gpio_x_open,
	.release 		= gpio_x_release,
	.poll 			= gpio_x_poll,
};
static int GPIOInit(void)
{
	int result = 0;
	
	g_gpio_ctx.c_dev = 0;
	g_gpio_ctx.pin = 0;
    
	result = alloc_chrdev_region(&g_gpio_ctx.dev, 0, 1, GPIO_DRV_NAME);
	if(result < 0){
		LREP("alloc_chrdev_region fail %d\r\n", result);
		return -1;
	}
	g_gpio_ctx.major = MAJOR(g_gpio_ctx.dev);
	LREP("major %d\r\n", g_gpio_ctx.major);
	
	g_gpio_class = class_create(THIS_MODULE, GPIO_DRV_NAME);
	device_create(g_gpio_class, NULL, MKDEV(g_gpio_ctx.major, 0), NULL, GPIO_DRV_NAME);
	// register file operations
	g_gpio_ctx.c_dev = cdev_alloc();
	cdev_init(g_gpio_ctx.c_dev, &gpio_fops);
	result = cdev_add(g_gpio_ctx.c_dev, g_gpio_ctx.dev, 1);
	if(result < 0){
		LREP_WARN("cdev_add fail %d\r\n", result);
		unregister_chrdev(g_gpio_ctx.major, GPIO_DRV_NAME);
		unregister_chrdev_region(g_gpio_ctx.dev, 1);	
		return -1;
	}
	// mapping device physical memory to virtual memory	of driver
	g_gpio_ctx.res = request_mem_region(GPIO_BASE_REG, GPIO_MEM_LEN, GPIO_DRV_NAME);
	if(!g_gpio_ctx.res){
		LREP_WARN("request_mem_region fail\r\n");
		cdev_del(g_gpio_ctx.c_dev);
		unregister_chrdev(g_gpio_ctx.major, GPIO_DRV_NAME);
		unregister_chrdev_region(g_gpio_ctx.dev, 1);	
		return -1;
	}
	g_gpio_ctx.mem = ioremap(GPIO_BASE_REG, GPIO_MEM_LEN);
	if(!g_gpio_ctx.mem){
		LREP_WARN("ioremap fail\r\n");
		release_mem_region(GPIO_BASE_REG, GPIO_MEM_LEN);
		cdev_del(g_gpio_ctx.c_dev);
		unregister_chrdev(g_gpio_ctx.major, GPIO_DRV_NAME);
		unregister_chrdev_region(g_gpio_ctx.dev, 1);
		return -1;
	}
	// register /dev/gpio_drv_x
	g_gpio_ctx.x_c_dev = 0;
    
	result = alloc_chrdev_region(&g_gpio_ctx.x_dev, 0, 255, GPIO_DRV_NAME "_");
	if(result < 0){
		LREP_WARN("alloc_chrdev_region fail %d\r\n", result);
		return -1;
	}
	g_gpio_ctx.x_major = MAJOR(g_gpio_ctx.x_dev);
	LREP("major x %d\r\n", g_gpio_ctx.major);
	
	g_gpio_class_x = class_create(THIS_MODULE, GPIO_DRV_NAME "_");
	
	//device_create(g_gpio_class_x, NULL, MKDEV(g_gpio_ctx.x_major, 0), NULL, GPIO_DRV_NAME "_" x);	
	
	g_gpio_ctx.x_c_dev = cdev_alloc();
	cdev_init(g_gpio_ctx.x_c_dev, &gpio_x_fops);
	result = cdev_add(g_gpio_ctx.x_c_dev, g_gpio_ctx.x_dev, 1);
	if(result < 0){
		LREP_WARN("cdev_add fail %d\r\n", result);
		unregister_chrdev(g_gpio_ctx.x_major, GPIO_DRV_NAME "_");
		unregister_chrdev_region(g_gpio_ctx.x_dev, 1);	
		return -1;
	}
	return result;
}
static void GPIOExit(void)
{
	iounmap(g_gpio_ctx.mem);
	release_mem_region(GPIO_BASE_REG, GPIO_MEM_LEN);
	cdev_del(g_gpio_ctx.c_dev);
	unregister_chrdev(g_gpio_ctx.major, GPIO_DRV_NAME);
	unregister_chrdev_region(g_gpio_ctx.dev, 1);	
	class_destroy(g_gpio_class);
	
	cdev_del(g_gpio_ctx.x_c_dev);
	unregister_chrdev(g_gpio_ctx.x_major, GPIO_DRV_NAME "_");
	unregister_chrdev_region(g_gpio_ctx.x_dev, 1);	
	class_destroy(g_gpio_class_x);
	
	if(g_gpio_ctx.pin){
		struct GPIO_PIN_CONTEXT* ptr = g_gpio_ctx.pin;
		struct GPIO_PIN_CONTEXT* ptr_next;
		while(ptr){
			ptr_next = ptr->next;
			if(ptr->irq >= 0) free_irq(ptr->irq, ptr);
			kfree(ptr);
			ptr = ptr_next;
		}
	}
	
}

module_init(GPIOInit);
module_exit(GPIOExit);

MODULE_AUTHOR("BinhNT");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");

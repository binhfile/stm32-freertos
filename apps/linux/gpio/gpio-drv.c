/** @file   gpio-driver.c
 *  @brief 	Driver for GPIO
 * /dev/gpio_drv	Open pinX
 * /dev/gpio_drv_x	PinX driver
 ******************************************************************************/

/***************************** Include Files *********************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
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
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/signal.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "gpio.h"
/************************** Constant Definitions *****************************/
#define GPIO_BASE_REG	(0x3F200000)
#define GPIO_MEM_LEN	(0xB0)
#define GPIO_DRV_NAME	"gpio_drv"
// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)

#define W_REG(reg, val) iowrite32(((u32)(val)), (g_gpio_ctx.mem+(reg)));
#define R_REG(reg) 		ioread32((g_gpio_ctx.mem+(reg)))

#define INP_GPIO(g) 		W_REG( (4*((g)/10)), (R_REG((4*((g)/10)))) & (~((u32)7<<(((g)%10)*3))) )
#define OUT_GPIO(g) 		W_REG( (4*((g)/10)), (R_REG((4*((g)/10)))) |  ((u32)1<<(((g)%10)*3)) )

#define GPIO_SET(g)			W_REG( (0x1C + (4*((g)/32))), (R_REG((0x1C + (4*((g)/32))))) | ((u32)1 << (g)%32) )
#define GPIO_CLR(g)			W_REG( (0x28 + (4*((g)/32))), (R_REG((0x28 + (4*((g)/32))))) | ((u32)1 << (g)%32) )

#define GPIO_READ(g) 		( (R_REG((0x34 + (4*((g)/32)))) >> ((g)%32)) & 1)

#define GPIO_SET_INTR_RISING(g)			W_REG( 0x4C + (4*((g)/32)), (R_REG(0x4C + (4*((g)/32)))) | ((u32)1 << (g)%32) )
#define GPIO_SET_INTR_FALLING(g)		W_REG( 0x58 + (4*((g)/32)), (R_REG(0x58 + (4*((g)/32)))) | ((u32)1 << (g)%32) )
#define GPIO_SET_INTR_NONE(g)			{\
	W_REG( 0x4C + (4*(g)/32), (R_REG(0x4C + (4*((g)/32)))) & (~((u32)1 << (g)%32)) );\
	W_REG( 0x58 + (4*(g)/32), (R_REG(0x58 + (4*((g)/32)))) & (~((u32)1 << (g)%32)) );\
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
	#if 0
	unsigned int		*mem;
	#else
	void				*mem;
	#endif
	int major;
	
	struct GPIO_PIN_CONTEXT	*pin;	
};

/************************** Variable Definitions *****************************/
struct GPIO_CONTEXT	g_gpio_ctx;
static struct class *g_gpio_class;
/***************** Macros (Inline Functions) Definitions *********************/
/*****************************************************************************/
static irqreturn_t gpio_x_isr (int irq, void *dev_id, struct pt_regs *regs)
{
	struct GPIO_PIN_CONTEXT *ctx = NULL;
	//unsigned long flags;
	
	//local_irq_save(flags);
	
	ctx = (struct GPIO_PIN_CONTEXT *)dev_id;
	if(!ctx){
		LREP("Interrupt ctx null\r\n");
		 return 0;
	}
	
	LREP("Interrupt [%d] for device was triggered !.\n", irq);
	
	ctx->state = 1;
	wake_up_interruptible(&ctx->wait_queue);
	
	//local_irq_restore(flags);
	return IRQ_HANDLED;
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
	int ret = -1;
	unsigned char buff[128] = {0};
	unsigned long  	irqflags = IRQF_TRIGGER_NONE;
	
	if(intr == GPIO_INTR_NONE) {
		GPIO_SET_INTR_NONE(pin);
	}
	else if(intr == GPIO_INTR_RISING) {
		irqflags = IRQF_TRIGGER_RISING;
	}
	else if(intr == GPIO_INTR_FALLING){
		 irqflags = IRQF_TRIGGER_FALLING;
	}
	else if(intr == GPIO_INTR_BOTH){
		irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
	}
	if(intr != GPIO_INTR_NONE){
		ctx = gpio_pin_to_ctx(pin);
		if(ctx){
			if(ctx->irq == -1){
				snprintf(buff, 127, GPIO_DRV_NAME "_%d", pin);
				if (gpio_request(pin, buff)) {
				  printk("GPIO request faiure: %s\n", buff);
				  return ret;
				}
				
				ctx->irq = gpio_to_irq(pin);
				ret = request_irq(ctx->irq, (irq_handler_t ) gpio_x_isr, irqflags, ctx->name, ctx);
				if(ret) LREP_WARN("request_irq %d fail %d\r\n", ctx->irq, ret);
				else{
					init_waitqueue_head(&ctx->wait_queue);
					if(intr == GPIO_INTR_RISING) {
						GPIO_SET_INTR_RISING(pin);
					}
					if(intr == GPIO_INTR_FALLING){
						 GPIO_SET_INTR_FALLING(pin);
					}
					else if(intr == GPIO_INTR_BOTH){
						GPIO_SET_INTR_RISING(pin);
						GPIO_SET_INTR_FALLING(pin);
					}
				}
			}
		}else LREP_WARN("ctx is null\r\n");
	}
	return 0;
}
unsigned char gpio_pin_read(int pin){
	return GPIO_READ(pin);
}
unsigned char gpio_pin_write(int pin, unsigned char val){
	if(val == 0){
		 GPIO_CLR(pin);
	 }
	else  {
		GPIO_SET(pin);
	}
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
	
	ucval = gpio_pin_read(minor);
	if(count < 0) count = 0;	
	if(copy_to_user(buff, (const void*)&ucval, 1)) return -1;
    return 1;
}
static ssize_t gpio_x_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	unsigned char ucval = 0;
	int minor = iminor(file_inode(filp));
	
	if(count > 0){
		if(copy_from_user((void*)&ucval, (const void __user *)buff, 1)) return 0;
		gpio_pin_write(minor, ucval);
		return count;
	}	
    return -EINVAL;
}
static long gpio_x_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	int minor = iminor(file_inode(filp));
	int ret = -EINVAL;
	unsigned int *pui = (unsigned int*)arg;
	switch(cmd){
		case GPIO_IOC_EXPORT_PIN:{
			unsigned int pin = *((unsigned int*)arg);	
			struct GPIO_PIN_CONTEXT* ptr=0;
			struct GPIO_PIN_CONTEXT* pin_ctx ;
			int exist;
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
				ret = 0;
				break;
			}
			device_create(g_gpio_class, NULL, MKDEV(g_gpio_ctx.major, pin), NULL, GPIO_DRV_NAME "_%d", pin);
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
			ret = 0;
			break;
		}
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
/*File operations of device*/
static struct file_operations gpio_fops = {
	.owner 			= THIS_MODULE,
	.read 			= gpio_x_read,
	.write 			= gpio_x_write,
	.unlocked_ioctl = gpio_x_ioctl,
	.open 			= gpio_x_open,
	.release 		= gpio_x_release,
	.poll 			= gpio_x_poll
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
	result = cdev_add(g_gpio_ctx.c_dev, g_gpio_ctx.dev, 255);
	if(result < 0){
		LREP_WARN("cdev_add fail %d\r\n", result);
		unregister_chrdev(g_gpio_ctx.major, GPIO_DRV_NAME);
		unregister_chrdev_region(g_gpio_ctx.dev, 255);	
		return -1;
	}
	// mapping device physical memory to virtual memory	of driver
	g_gpio_ctx.res = request_mem_region(GPIO_BASE_REG, GPIO_MEM_LEN, GPIO_DRV_NAME);
	if(!g_gpio_ctx.res){
		LREP_WARN("request_mem_region fail\r\n");
		cdev_del(g_gpio_ctx.c_dev);
		unregister_chrdev(g_gpio_ctx.major, GPIO_DRV_NAME);
		unregister_chrdev_region(g_gpio_ctx.dev, 255);	
		return -1;
	}
	g_gpio_ctx.mem = ioremap(GPIO_BASE_REG, GPIO_MEM_LEN);
	if(!g_gpio_ctx.mem){
		LREP_WARN("ioremap fail\r\n");
		release_mem_region(GPIO_BASE_REG, GPIO_MEM_LEN);
		cdev_del(g_gpio_ctx.c_dev);
		unregister_chrdev(g_gpio_ctx.major, GPIO_DRV_NAME);
		unregister_chrdev_region(g_gpio_ctx.dev, 255);
		return -1;
	}	
	LREP("map to %p\r\n", g_gpio_ctx.mem);
	return result;
}
static void GPIOExit(void)
{
	iounmap(g_gpio_ctx.mem);
	release_mem_region(GPIO_BASE_REG, GPIO_MEM_LEN);
	cdev_del(g_gpio_ctx.c_dev);
	unregister_chrdev(g_gpio_ctx.major, GPIO_DRV_NAME);
	unregister_chrdev_region(g_gpio_ctx.dev, 255);	
	class_destroy(g_gpio_class);
	
	if(g_gpio_ctx.pin){
		struct GPIO_PIN_CONTEXT* ptr = g_gpio_ctx.pin;
		struct GPIO_PIN_CONTEXT* ptr_next;
		while(ptr){
			ptr_next = ptr->next;
			if(ptr->irq >= 0) {
				gpio_free(ptr->pin);
				free_irq(ptr->irq, ptr);
			}
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

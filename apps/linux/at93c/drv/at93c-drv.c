/******************************************************************************/

/***************************** Include Files *********************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/delay.h>

#include <linux/gpio.h>


#include "../at93c.h"
#include "at93c_def.h"
/************************** Constant Definitions *****************************/
#define AT93C_DRV_NAME          "at93c"
#define LREP(x, args...)        printk(KERN_ALERT   AT93C_DRV_NAME ":" x , ##args)
#define LREP_WARN(x, args...)   printk(KERN_ALERT   AT93C_DRV_NAME ": %d@%s " x , __LINE__, __FILE__, ##args)
/**************************** Type Definitions *******************************/

#define AT93C_DRV_GPIO_SCK      0
#define AT93C_DRV_GPIO_MOSI     1
#define AT93C_DRV_GPIO_MISO     2
#define AT93C_DRV_GPIO_CS       3

struct at93c_device_info{
    u8      is_opened;
    loff_t  offset;
};

struct AT93C_CONTEXT
{
    dev_t           dev;
    struct cdev     *c_dev;
    struct class    *c_class;

    struct at93c_device_info    *device_info;
    int             device_count;
};
/************************** Variable Definitions *****************************/
struct AT93C_CONTEXT    g_at93c_ctx;
struct at93c_board_info{
    struct gpio gpios[4];
    int         size;
    char*       name;
};

struct at93c_board_info g_at93c_board_info[1] = {
    {
        .gpios = {
            { 23, GPIOF_OUT_INIT_LOW,  "sck"},
            { 24, GPIOF_OUT_INIT_LOW,  "mosi"},
            { 25, GPIOF_IN,            "miso"},
            { 22, GPIOF_OUT_INIT_HIGH, "cs"},
         },
         .size = AT93C66_SIZE,
         .name = "at93c66",
    },
};

/***************** Macros (Inline Functions) Definitions *********************/

#define DRV_AT93C_SET_GPIO(num, val)    gpio_set_value(num, ((val == 0) ? 0 : 1))
#define DRV_AT93C_DELAY_SCK() {volatile int __i = 45; while(__i--){}}    // 1us
#define DRV_AT93C_DELAY_CS() {volatile int __i = 45; while(__i--){}}        // 250 ns
#define DRV_AT93C_R_BIT(minor, ret) {\
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio , 1); \
    DRV_AT93C_DELAY_SCK(); \
    ret = gpio_get_value(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MISO].gpio);\
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0); \
    DRV_AT93C_DELAY_SCK(); \
}
#define DRV_AT93C_WAIT_W_DONE(minor) {\
    u8 timeout = 1000;\
    while(timeout -- && gpio_get_value(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MISO].gpio) == 0){\
        ndelay(1000*1000);\
    }\
}
#define DRV_AT93C_W_BIT(minor, val)    {\
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MOSI].gpio, val); \
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 1); \
    DRV_AT93C_DELAY_SCK(); \
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0); \
    DRV_AT93C_DELAY_SCK(); \
}
/*****************************************************************************/
static u8 at93c_enable_write(struct AT93C_CONTEXT* ctx, int minor){
    // sck = 0
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MOSI].gpio, 0);
    // cs = 1
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    // write 100 110000000
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);

    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    // cs = 0
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    return 0;
}
static u8 at93c_disable_write(struct AT93C_CONTEXT* ctx, int minor){
    // sck = 0
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MOSI].gpio, 0);
    // cs = 1
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    // write 100 000000000
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);

    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    // cs = 0
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    return 0;
}
static u8 at93c_erase_all(struct AT93C_CONTEXT* ctx, int minor){
    at93c_enable_write(ctx, minor);

    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MOSI].gpio, 0);
    // cs = 1
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    // write 100 100000000
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);

    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);
    // cs = 0
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_DELAY_CS();
    //wait done
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_WAIT_W_DONE(minor);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_DELAY_CS();
    return 0;
}
static u8 at93c_erase(struct AT93C_CONTEXT* ctx, int minor, u16 address){
    at93c_enable_write(ctx, minor);
    // sck = 0
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MOSI].gpio, 0);
    // cs = 1
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    // write 111 A8-A0
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 1);

    DRV_AT93C_W_BIT(minor, (((u16)address >> 8) & (u16)0x0001));//8
    DRV_AT93C_W_BIT(minor, (((u16)address >> 7) & (u16)0x0001));//7
    DRV_AT93C_W_BIT(minor, (((u16)address >> 6) & (u16)0x0001));//6
    DRV_AT93C_W_BIT(minor, (((u16)address >> 5) & (u16)0x0001));//5
    DRV_AT93C_W_BIT(minor, (((u16)address >> 4) & (u16)0x0001));//4
    DRV_AT93C_W_BIT(minor, (((u16)address >> 3) & (u16)0x0001));//3
    DRV_AT93C_W_BIT(minor, (((u16)address >> 2) & (u16)0x0001));//2
    DRV_AT93C_W_BIT(minor, (((u16)address >> 1) & (u16)0x0001));//1
    DRV_AT93C_W_BIT(minor, (((u16)address) & (u16)0x0001));//0
    // cs = 0
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_DELAY_CS();
    //wait done
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_WAIT_W_DONE(minor);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_DELAY_CS();
    return 0;
}
static u8 at93c_fill(struct AT93C_CONTEXT* ctx, int minor, u8 pattern){
    at93c_enable_write(ctx, minor);
    // sck = 0
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MOSI].gpio, 0);
    // cs = 1
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    // write 111 A8-A0
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 0);
    DRV_AT93C_W_BIT(minor, 0);

    DRV_AT93C_W_BIT(minor, 0);//8
    DRV_AT93C_W_BIT(minor, 1);//7
    DRV_AT93C_W_BIT(minor, 0);//6
    DRV_AT93C_W_BIT(minor, 0);//5
    DRV_AT93C_W_BIT(minor, 0);//4
    DRV_AT93C_W_BIT(minor, 0);//3
    DRV_AT93C_W_BIT(minor, 0);//2
    DRV_AT93C_W_BIT(minor, 0);//1
    DRV_AT93C_W_BIT(minor, 0);//0
    // D7-D0
    DRV_AT93C_W_BIT(minor, ((pattern & (((u8)1) << 7)) != 0));//7
    DRV_AT93C_W_BIT(minor, ((pattern & (((u8)1) << 6)) != 0));//6
    DRV_AT93C_W_BIT(minor, ((pattern & (((u8)1) << 5)) != 0));//5
    DRV_AT93C_W_BIT(minor, ((pattern & (((u8)1) << 4)) != 0));//4
    DRV_AT93C_W_BIT(minor, ((pattern & (((u8)1) << 3)) != 0));//3
    DRV_AT93C_W_BIT(minor, ((pattern & (((u8)1) << 2)) != 0));//2
    DRV_AT93C_W_BIT(minor, ((pattern & (((u8)1) << 1)) != 0));//1
    DRV_AT93C_W_BIT(minor, ((pattern & (((u8)1) << 0)) != 0));//0
    // cs = 0
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_DELAY_CS();
    //wait done
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_WAIT_W_DONE(minor);
    DRV_AT93C_DELAY_CS();
    return 0;
}
static int at93c_open(struct inode *inode, struct file *filp)
{
    int minor = iminor(file_inode(filp));

    if(g_at93c_ctx.device_info[minor].is_opened) return -1;

    g_at93c_ctx.device_info[minor].is_opened  = 1;
    g_at93c_ctx.device_info[minor].offset = 0;
    return 0;
}
static int at93c_release(struct inode *inode, struct file *filp)
{
    int minor = iminor(file_inode(filp));
    //LREP("close");
    g_at93c_ctx.device_info[minor].is_opened = 0;
    return 0;
}
static ssize_t at93c_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    int minor = iminor(file_inode(filp));
    u8 u8read, u8total;
    u8 *pu8, *mbuff;
    int ret;
    u16 address;

    if(count <= 0) return 0;
    if(count > g_at93c_board_info[minor].size) count = g_at93c_board_info[minor].size;

    mbuff = (u8*)kmalloc(sizeof(u8)*count, GFP_ATOMIC);
    pu8  = mbuff;
    address = g_at93c_ctx.device_info[minor].offset + *offp;

    ret = count;
    // sck = 0
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0);
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MOSI].gpio, 0);
    // cs = 1
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();
    // write 111 A8-A0
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 1);
    DRV_AT93C_W_BIT(minor, 0);

    DRV_AT93C_W_BIT(minor, (((u16)address >> 8) & (u16)0x0001));//8
    DRV_AT93C_W_BIT(minor, (((u16)address >> 7) & (u16)0x0001));//7
    DRV_AT93C_W_BIT(minor, (((u16)address >> 6) & (u16)0x0001));//6
    DRV_AT93C_W_BIT(minor, (((u16)address >> 5) & (u16)0x0001));//5
    DRV_AT93C_W_BIT(minor, (((u16)address >> 4) & (u16)0x0001));//4
    DRV_AT93C_W_BIT(minor, (((u16)address >> 3) & (u16)0x0001));//3
    DRV_AT93C_W_BIT(minor, (((u16)address >> 2) & (u16)0x0001));//2
    DRV_AT93C_W_BIT(minor, (((u16)address >> 1) & (u16)0x0001));//1
    DRV_AT93C_W_BIT(minor, (((u16)address) & (u16)0x0001));//0
    // read data
    while(ret > 0){
        u8total = 0;
        DRV_AT93C_R_BIT(minor, u8read);
        u8total |= (u8)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
        DRV_AT93C_R_BIT(minor, u8read);
        u8total |= (u8)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
        DRV_AT93C_R_BIT(minor, u8read);
        u8total |= (u8)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
        DRV_AT93C_R_BIT(minor, u8read);
        u8total |= (u8)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
        DRV_AT93C_R_BIT(minor, u8read);
        u8total |= (u8)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
        DRV_AT93C_R_BIT(minor, u8read);
        u8total |= (u8)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
        DRV_AT93C_R_BIT(minor, u8read);
        u8total |= (u8)((u8read == 1) ? 1 : 0); u8total =  u8total<<1;
        DRV_AT93C_R_BIT(minor, u8read);
        u8total |= (u8)((u8read == 1) ? 1 : 0);

        *pu8 = u8total;
        pu8++;
        ret--;
    }
    // cs = 0
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
    DRV_AT93C_DELAY_CS();
    DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
    DRV_AT93C_DELAY_CS();

    ret = copy_to_user(buff, mbuff, count);

    kfree(mbuff);

    g_at93c_ctx.device_info[minor].offset += *offp + count;

    return count;
}
static ssize_t at93c_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
    u8* pu8, *mbuff;
    u8 u8w;
    int ret;
    u16 address;
    int minor = iminor(file_inode(filp));

    if(count <= 0) return 0;
    if(count > g_at93c_board_info[minor].size) count = g_at93c_board_info[minor].size;

    mbuff = (u8*)kmalloc(sizeof(u8)*count, GFP_ATOMIC);
    ret = copy_from_user(mbuff, buff, count);
    pu8  = mbuff;
    address = g_at93c_ctx.device_info[minor].offset + *offp;

    ret = count;
    at93c_enable_write(&g_at93c_ctx, minor);

    while(ret > 0){
        u8w = *pu8;
        DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
        DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_SCK].gpio, 0);
        DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_MOSI].gpio, 0);
        DRV_AT93C_DELAY_CS();
        // cs = 1
        DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
        DRV_AT93C_DELAY_CS();
        // write 111 A8-A0
        DRV_AT93C_W_BIT(minor, 1);
        DRV_AT93C_W_BIT(minor, 0);
        DRV_AT93C_W_BIT(minor, 1);

        DRV_AT93C_W_BIT(minor, (((u16)address >> 8) & (u16)0x0001));//8
        DRV_AT93C_W_BIT(minor, (((u16)address >> 7) & (u16)0x0001));//7
        DRV_AT93C_W_BIT(minor, (((u16)address >> 6) & (u16)0x0001));//6
        DRV_AT93C_W_BIT(minor, (((u16)address >> 5) & (u16)0x0001));//5
        DRV_AT93C_W_BIT(minor, (((u16)address >> 4) & (u16)0x0001));//4
        DRV_AT93C_W_BIT(minor, (((u16)address >> 3) & (u16)0x0001));//3
        DRV_AT93C_W_BIT(minor, (((u16)address >> 2) & (u16)0x0001));//2
        DRV_AT93C_W_BIT(minor, (((u16)address >> 1) & (u16)0x0001));//1
        DRV_AT93C_W_BIT(minor, (((u16)address) & (u16)0x0001));//0
        // D7-D0
        DRV_AT93C_W_BIT(minor, ((u8w >> 7) & (u8)0x01));//7
        DRV_AT93C_W_BIT(minor, ((u8w >> 6) & (u8)0x01));//6
        DRV_AT93C_W_BIT(minor, ((u8w >> 5) & (u8)0x01));//5
        DRV_AT93C_W_BIT(minor, ((u8w >> 4) & (u8)0x01));//4
        DRV_AT93C_W_BIT(minor, ((u8w >> 3) & (u8)0x01));//3
        DRV_AT93C_W_BIT(minor, ((u8w >> 2) & (u8)0x01));//2
        DRV_AT93C_W_BIT(minor, ((u8w >> 1) & (u8)0x01));//1
        DRV_AT93C_W_BIT(minor, ((u8w) & 0x01));//0
        // cs = 0
        DRV_AT93C_DELAY_CS();
        DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
        DRV_AT93C_DELAY_CS();
        //wait done
        DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 1);
        DRV_AT93C_DELAY_CS();
        DRV_AT93C_WAIT_W_DONE(minor);
        DRV_AT93C_SET_GPIO(g_at93c_board_info[minor].gpios[AT93C_DRV_GPIO_CS].gpio, 0);
        DRV_AT93C_DELAY_CS();
        //
        ret--;
        pu8++;
        address++;
    }
    kfree(mbuff);
    g_at93c_ctx.device_info[minor].offset += *offp + count;
    return count;
}
static long at93c_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    int minor = iminor(file_inode(filp));
    long ret = -EINVAL;
    switch(cmd){
        case AT93C_IOC_WR_ERASE_ALL:{
            at93c_erase_all(&g_at93c_ctx, minor);
            ret = 0;
            break;
        }
        case AT93C_IOC_RD_SIZE:{
            unsigned int *val = (unsigned int*)arg;
            *val = g_at93c_board_info[minor].size;
            ret = 0;
            break;
        }
        case AT93C_IOC_RD_NAME:{
            unsigned char *val = (unsigned char*)arg;
            strncpy(val, g_at93c_board_info[minor].name, 31);
            ret = 0;
            break;
        }
        default:
            LREP_WARN("unknown cmd %X\r\n", cmd);
    }
    return ret;
}
static unsigned int at93c_poll(struct file *filp, struct poll_table_struct * wait)
{
    int ret = -1;

    return ret;
}
loff_t at93c_llseek (struct file *filp, loff_t off, int whence){
    int minor = iminor(file_inode(filp));
    loff_t newpos;

    switch(whence) {
      case 0: /* SEEK_SET */
        newpos = off;
        break;

      case 1: /* SEEK_CUR */
        newpos = g_at93c_ctx.device_info[minor].offset + off;
        break;

      case 2: /* SEEK_END */
        newpos = g_at93c_board_info[minor].size + off;
        break;

      default: /* can't happen */
        return -EINVAL;
    }
    if (newpos < 0) return -EINVAL;
    g_at93c_ctx.device_info[minor].offset = newpos;
    return newpos;
}
/*File operations of device*/
const char g_at93c_drv_name[] = "at93c";
static struct file_operations at93c_fops = {
    .owner  = THIS_MODULE,
    .read   = at93c_read,
    .write  = at93c_write,
    .unlocked_ioctl = at93c_ioctl,
    .open           = at93c_open,
    .release        = at93c_release,
    .poll   = at93c_poll,
    .llseek = at93c_llseek,
};
static int at93c_init(void)
{
    int result = 0;
    int i;
    char buffer[128];

    g_at93c_ctx.device_count = sizeof(g_at93c_board_info) / sizeof(struct at93c_board_info);

    if(g_at93c_ctx.device_count <= 0) {
        LREP_WARN("no device board info\r\n");
        return -1;
    }
    g_at93c_ctx.device_info = (struct at93c_device_info*)kmalloc(sizeof(struct at93c_device_info) * g_at93c_ctx.device_count, GFP_ATOMIC);
    for(i = 0; i < g_at93c_ctx.device_count; i++){
        g_at93c_ctx.device_info[i].is_opened = 0;
        g_at93c_ctx.device_info[i].offset = 0;
    }
    g_at93c_ctx.c_dev             = 0;
    // mk dev
    g_at93c_ctx.dev = MKDEV(0, 0);
    result = alloc_chrdev_region(&g_at93c_ctx.dev, 0, g_at93c_ctx.device_count, AT93C_DRV_NAME);
    if(result < 0){
        LREP_WARN("alloc_chrdev_region fail %d\r\n", result);
        kfree(g_at93c_ctx.device_info);
        return -1;
    }
    // register file operations
    g_at93c_ctx.c_dev = cdev_alloc();
    cdev_init(g_at93c_ctx.c_dev, &at93c_fops);
    result = cdev_add(g_at93c_ctx.c_dev, g_at93c_ctx.dev, g_at93c_ctx.device_count);
    if(result < 0){
        LREP_WARN("cdev_add fail %d\r\n", result);
        goto INIT_FAIL_1;
    }
    g_at93c_ctx.c_class = class_create(THIS_MODULE, AT93C_DRV_NAME);

    for(i = 0; i < g_at93c_ctx.device_count; i++){
        snprintf(buffer, 127, AT93C_DRV_NAME ".%d", i);
        if(!device_create(g_at93c_ctx.c_class, NULL, MKDEV(MAJOR(g_at93c_ctx.dev), i), NULL, buffer)){
            LREP_WARN("device_create fail\r\n");
            goto INIT_FAIL_2;
        }
        // gpio
        result = gpio_request_array(g_at93c_board_info[i].gpios, ARRAY_SIZE(g_at93c_board_info[i].gpios));
        if (result){
            LREP_WARN("gpio_request_array fail %d\r\n", result);
            goto INIT_FAIL_2;
        }
        LREP("register device %s\r\n", buffer);
    }

    LREP("init done\r\n");
    result = 0;
    goto INIT_DONE;
INIT_FAIL_2:
    class_destroy(g_at93c_ctx.c_class);
    cdev_del(g_at93c_ctx.c_dev);
INIT_FAIL_1:
    kfree(g_at93c_ctx.device_info);
    unregister_chrdev_region(g_at93c_ctx.dev, 1);
    result = -1;
INIT_DONE:
    return result;
}
static void at93c_exit(void)
{
    int i;
    for(i = 0; i < g_at93c_ctx.device_count; i++){
        gpio_free_array(g_at93c_board_info[i].gpios, ARRAY_SIZE(g_at93c_board_info[i].gpios));
    }
    device_destroy(g_at93c_ctx.c_class, g_at93c_ctx.dev);
    class_destroy(g_at93c_ctx.c_class);
    cdev_del(g_at93c_ctx.c_dev);
    unregister_chrdev_region(g_at93c_ctx.dev, g_at93c_ctx.device_count);
    kfree(g_at93c_ctx.device_info);
    LREP("exit\r\n");
}

module_init(at93c_init);
module_exit(at93c_exit);

MODULE_AUTHOR("BinhNT");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0");

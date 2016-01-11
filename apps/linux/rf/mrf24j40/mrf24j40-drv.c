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
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>

#include <linux/spi/spi.h>
#include <linux/gpio.h>


#include "../rf_mac.h"
#include "mrf24j40_reg.h"
/************************** Constant Definitions *****************************/
#define MRF24J40_DRV_NAME		"mrf24j40"
#define LREP(x, args...)  		printk(KERN_ALERT   MRF24J40_DRV_NAME ":" x , ##args)
#define LREP_WARN(x, args...)  	printk(KERN_ALERT	MRF24J40_DRV_NAME ": %d@%s " x , __LINE__, __FILE__, ##args)
/**************************** Type Definitions *******************************/

#define DRV_MRF24J40_GPIO_RESET		0
#define DRV_MRF24J40_GPIO_INTR		1

#define DRV_MRF24J40_WRITE_COUNT	3
#define DRV_MRF24J40_READ_COUNT		3
#define DRV_MRF24J40_WRITE_PAYLOAD_MAX_LENGTH   (100)
#define DRV_MRF24J40_READ_PAYLOAD_MAX_LENGTH    (144)

struct mrf24j40_spi{
	int spi_bus_num;
	int spi_chip_select;
};
struct mrf24j40_write_node{
	struct rf_mac_write_packet_header header;
	u8 flags;
	u8 data_len;
	u8 data[DRV_MRF24J40_WRITE_PAYLOAD_MAX_LENGTH];
};
struct mrf24j40_read_node{
	u8 flags;
	u8 data_len;
	u8 data[DRV_MRF24J40_READ_PAYLOAD_MAX_LENGTH];
};
enum mrf24j40_mode{
	mrf24j40_mode_rx = 0,
	mrf24j40_mode_tx
};
struct MRF24J40_CONTEXT
{
	dev_t 				dev;
	struct cdev 		*c_dev;
	struct class 		*c_class;

	struct spi_device *spi_device;
	int					intr_irq;
	u8					is_opened;
	u8					is_write_verify;

	struct gpio 		gpios[2];
	struct mrf24j40_spi spi;

	struct mrf24j40_write_node write_list[DRV_MRF24J40_WRITE_COUNT];
	struct mrf24j40_read_node read_list[DRV_MRF24J40_READ_COUNT];
	int					current_tx_index;

	spinlock_t				   lock;
	struct workqueue_struct    *wq;
	u8					tx_seq;
	enum mrf24j40_mode	mode;

	wait_queue_head_t	wait_queue;

	u8					short_address[2];
	u8					long_address[8];
	u8					pan_id[2];
};
/************************** Variable Definitions *****************************/
struct MRF24J40_CONTEXT	g_mrf24j40_ctx = {
		.spi = {
				.spi_bus_num = 0,
				.spi_chip_select = 0,
		},
		.gpios = {
				{ 17, GPIOF_OUT_INIT_HIGH, "rf-reset"},
				{ 27, GPIOF_IN,  "rf-intr" },
		},
		.is_write_verify = 0,
};
/***************** Macros (Inline Functions) Definitions *********************/
/*****************************************************************************/
static u8 mrf24j40_get_long_reg(u16 address){
	struct spi_message msg;
	struct spi_transfer tr;
	u8 tx[3], rx[3];

	spi_message_init(&msg);
	memset(&tr, 0, sizeof(tr));

	tx[0] = ((address >> 3)&0x7F) | 0x80;
	tx[1] = ((address << 5)&0xE0);
	tx[2] = 0;
	rx[2] = 0;

	tr.tx_buf 	= &tx[0];
	tr.rx_buf 	= &rx[0];
	tr.len 		= 3;
	spi_message_add_tail(&tr, &msg);

	spi_sync(g_mrf24j40_ctx.spi_device, &msg);

	return rx[2];
}
static u8 mrf24j40_get_short_reg(u8 address){
	struct spi_message msg;
	struct spi_transfer tr;
	u8 tx[2], rx[2];

	spi_message_init(&msg);
	memset(&tr, 0, sizeof(tr));

	tx[0] = address;
	tx[1] = 0;
	rx[1] = 0;

	tr.tx_buf 	= &tx[0];
	tr.rx_buf 	= &rx[0];
	tr.len 		= 2;
	spi_message_add_tail(&tr, &msg);

	spi_sync(g_mrf24j40_ctx.spi_device, &msg);

	return rx[1];
}
static u8 mrf24j40_set_long_reg(u16 address, u8 val){
	struct spi_message msg;
	struct spi_transfer tr;
	u8 tx[3];

	spi_message_init(&msg);
	memset(&tr, 0, sizeof(tr));

	tx[0] = (((address >> 3))&0x7F) | 0x80;
	tx[1] = (((address << 5))&0xE0) | 0x10;
	tx[2] = val;

	tr.tx_buf 	= &tx[0];
	tr.len 		= 3;
	spi_message_add_tail(&tr, &msg);

	spi_sync(g_mrf24j40_ctx.spi_device, &msg);

	if(g_mrf24j40_ctx.is_write_verify){
		tx[0] = mrf24j40_get_long_reg(address);
		if(tx[0] != val){
			LREP_WARN("set reg %04X=%02X failed, read back %02X\r\n",
							address, val, tx[0]);
		}
		else{
			LREP("set reg %04X=%02X done\r\n",
							address, val);
		}
	}

	return 0;
}
static u8 mrf24j40_set_short_reg(u8 address, u8 val){
	struct spi_message msg;
	struct spi_transfer tr;
	u8 tx[2];

	spi_message_init(&msg);
	memset(&tr, 0, sizeof(tr));

	tx[0] = address;
	tx[1] = val;

	tr.tx_buf 	= &tx[0];
	tr.len 		= 2;
	spi_message_add_tail(&tr, &msg);

	spi_sync(g_mrf24j40_ctx.spi_device, &msg);

	if(g_mrf24j40_ctx.is_write_verify){
		if(address != MRF24J40_WRITE_SOFTRST){
			tx[0] = mrf24j40_get_short_reg(address & (~((uint8_t)0x01)));
			if(address == MRF24J40_WRITE_RXFLUSH){
				tx[0] &= ((uint8_t)0x6E);
				val &= ~((uint8_t)0x01);
			}
			else if(address == MRF24J40_WRITE_BBREG6) tx[0] &= (uint8_t)(0x40);
			if(tx[0] != val){
				LREP_WARN("set reg %02X=%02X failed, read back %02X\r\n",
								address, val, tx[0]);
			}
			else{
				LREP("set reg %02X=%02X done\r\n",
								address, val);
			}
		}
	}

	return 0;
}

static u8 mrf24j40_hard_reset(void){
	gpio_set_value(g_mrf24j40_ctx.gpios[DRV_MRF24J40_GPIO_RESET].gpio, 0);
	mdelay(50);
	gpio_set_value(g_mrf24j40_ctx.gpios[DRV_MRF24J40_GPIO_RESET].gpio, 1);
	mdelay(50);
	return 0;
}
static u8 mrf24j40_soft_reset(void){
	u8 i;
	u8 timeout = 100;
	mrf24j40_set_short_reg(MRF24J40_WRITE_SOFTRST, 0x07);
	do{
		i = mrf24j40_get_short_reg(MRF24J40_READ_SOFTRST);
		if(((i & 0x07) == (uint8_t) 0x00)) break;
		mdelay(10);
		timeout -= 10;
	}while(timeout > 0);
	return 0;
}
static u8 mrf24j40_set_channel(u8 channel)
{
    if (channel < 11 || channel > 26)
    {
        return -1;
    }
    mrf24j40_set_long_reg(MRF24J40_RFCTRL0, ((channel - 11) << 4) | 0x03);
    mrf24j40_set_short_reg(MRF24J40_WRITE_RFCTL, 0x04);
    mrf24j40_set_short_reg(MRF24J40_WRITE_RFCTL, 0x00);
    return 0;
}
static void mrf24j40_set_short_address(u8 *address){
	mrf24j40_set_short_reg(MRF24J40_WRITE_SADRL, address[0]);
	mrf24j40_set_short_reg(MRF24J40_WRITE_SADRH, address[1]);
	g_mrf24j40_ctx.short_address[0] = address[0];
	g_mrf24j40_ctx.short_address[1] = address[1];
}
static void mrf24j40_set_long_address(u8 *address){
	int i;
	for (i = 0; i < (uint8_t) 8; i++)
	{
		mrf24j40_set_short_reg(MRF24J40_WRITE_EADR0 + i * 2, address[i]);
		g_mrf24j40_ctx.long_address[i] = address[i];
	}
}
static void mrf24j40_set_pan_id(u8 *panid){
	mrf24j40_set_short_reg(MRF24J40_WRITE_PANIDL, panid[0]);
	mrf24j40_set_short_reg(MRF24J40_WRITE_PANIDH, panid[1]);
	g_mrf24j40_ctx.pan_id[0] = panid[0];
	g_mrf24j40_ctx.pan_id[1] = panid[1];
}
void mrf24j40_initialize(void){
	u8 i;
	u8 timeout = 100;
	mrf24j40_hard_reset();
	mrf24j40_soft_reset();

   /* flush the RX fifo */
	mrf24j40_set_short_reg(MRF24J40_WRITE_RXFLUSH, 0x01);

	/* Program the short MAC Address, 0xffff */
	mrf24j40_set_short_reg(MRF24J40_WRITE_SADRL, 0xff);
	mrf24j40_set_short_reg(MRF24J40_WRITE_SADRH, 0xff);
	mrf24j40_set_short_reg(MRF24J40_WRITE_PANIDL, 0xff);
	mrf24j40_set_short_reg(MRF24J40_WRITE_PANIDH, 0xff);

	/* Program Long MAC Address*/
	for (i = 0; i < (uint8_t) 8; i++)
	{
		mrf24j40_set_short_reg(MRF24J40_WRITE_EADR0 + i * 2, 0xff);
	}

	/* setup */
	mrf24j40_set_long_reg(MRF24J40_RFCTRL2, 0x80);

#if defined(ENABLE_PA_LNA)
#if defined(MRF24J40MB)
	// There are special MRF24J40 transceiver output power
	// setting for Microchip MRF24J40MB module.
#if APPLICATION_SITE == EUROPE
	// MRF24J40 output power set to be -14.9dB
	mrf24j40_set_long_reg(MRF24J40_RFCTRL3, 0x70);
#else
	// MRF24J40 output power set to be -1.9dB
	mrf24j40_set_long_reg(MRF24J40_RFCTRL3, 0x18);
#endif
#elif defined(MRF24J40MC)
	// MRF24J40 output power set to be -3.7dB for MRF24J40MB
	mrf24j40_set_long_reg(MRF24J40_RFCTRL3, 0x28);
#else
	// power level set to be 0dBm, must adjust according to
	// FCC/IC/ETSI requirement
	mrf24j40_set_long_reg(MRF24J40_RFCTRL3, 0x00);
#endif
#else
	// power level to be 0dBm
	mrf24j40_set_long_reg(MRF24J40_RFCTRL3, 0x00);
#endif

	/* program RSSI ADC with 2.5 MHz clock */
	mrf24j40_set_long_reg(MRF24J40_RFCTRL6, 0x90);

	mrf24j40_set_long_reg(MRF24J40_RFCTRL7, 0x80);

	mrf24j40_set_long_reg(MRF24J40_RFCTRL8, 0x10);

	mrf24j40_set_long_reg(MRF24J40_SCLKDIV, 0x21);

	/* Program CCA mode using RSSI */
	mrf24j40_set_short_reg(MRF24J40_WRITE_BBREG2, 0x80);
	/* Enable the packet RSSI */
	mrf24j40_set_short_reg(MRF24J40_WRITE_BBREG6, 0x40);
	/* Program CCA, RSSI threshold values */
	mrf24j40_set_short_reg(MRF24J40_WRITE_RSSITHCCA, 0x60);

#if defined(ENABLE_PA_LNA)

#if defined(MRF24J40MC)
	mrf24j40_set_short_reg(MRF24J40_WRITE_GPIODIR, 0x08);
	mrf24j40_set_short_reg(MRF24J40_WRITE_GPIO, 0x08);
#endif
	mrf24j40_set_long_reg(MRF24J40_TESTMODE, 0x0F);

#endif
	mrf24j40_set_short_reg(MRF24J40_WRITE_FFOEN, 0x98);
	mrf24j40_set_short_reg(MRF24J40_WRITE_TXPEMISP, 0x95);

	// wait until the MRF24J40 in receive mode
	do
	{
		i = mrf24j40_get_long_reg(MRF24J40_RFSTATE);
		msleep(1);
		timeout--;
	}
	while (((i&0xA0) != 0xA0) && (timeout > 0));
	//if(timeout == 0) LREP_WARN("timeout\r\n");

	mrf24j40_set_short_reg(MRF24J40_WRITE_INTMSK, 0xE6);

#ifdef ENABLE_INDIRECT_MESSAGE
	mrf24j40_set_short_reg(MRF24J40_WRITE_ACKTMOUT, 0xB9);
#endif

	// Make RF communication stable under extreme temperatures
	mrf24j40_set_long_reg(MRF24J40_RFCTRL0, 0x03);
	mrf24j40_set_long_reg(MRF24J40_RFCTRL1, 0x02);
	mrf24j40_set_channel(11);
	// Define TURBO_MODE if more bandwidth is required
	// to enable radio to operate to TX/RX maximum
	// 625Kbps
	if(0){
		mrf24j40_set_short_reg(MRF24J40_WRITE_BBREG0, 0x01);
		mrf24j40_set_short_reg(MRF24J40_WRITE_BBREG3, 0x38);
		mrf24j40_set_short_reg(MRF24J40_WRITE_BBREG4, 0x5C);

		mrf24j40_set_short_reg(MRF24J40_WRITE_RFCTL, 0x04);
		mrf24j40_set_short_reg(MRF24J40_WRITE_RFCTL, 0x00);
	}
	return;
}

static void mrf24j40_write_packet(struct mrf24j40_write_node* pkt){
	struct mrf24j40_frm_ctrl frmCtrl;
	u8 hdr_len, i;
	u16 ram_addr;

	frmCtrl.bits.Val = 0;
	frmCtrl.bits.frmType = mrf24j40_frmtype_data;

	// inner PAN
	hdr_len = 2+1+2;// frmCrl(2)+seq(1)+destPANId(2)
	frmCtrl.bits.intraPAN = pkt->header.flags.bits.intra_pan;
	//altDestAddr = 1
	//altSrcAddr = 1
	// ACK req
	if(pkt->header.flags.bits.ack_req && (pkt->header.flags.bits.broadcast == 0))
		frmCtrl.bits.ackReq = 1;

	if(pkt->header.flags.bits.dest_addr_64bit)
		frmCtrl.bits.destAddrMode = mrf24j40_addrmode_64bit;
	else
		frmCtrl.bits.destAddrMode = mrf24j40_addrmode_16bit;
	if(pkt->header.flags.bits.src_addr_64bit)
		frmCtrl.bits.srcAddrMode = mrf24j40_addrmode_64bit;
	else
		frmCtrl.bits.srcAddrMode = mrf24j40_addrmode_16bit;

	if(frmCtrl.bits.destAddrMode == mrf24j40_addrmode_16bit) hdr_len+=2;
	else hdr_len +=8;
	if(frmCtrl.bits.srcAddrMode == mrf24j40_addrmode_16bit) hdr_len+=2;
	else hdr_len +=8;

	ram_addr = 0;
	mrf24j40_set_long_reg( ram_addr++, hdr_len);
	mrf24j40_set_long_reg( ram_addr++, hdr_len + pkt->data_len);
	mrf24j40_set_long_reg( ram_addr++, (frmCtrl.bits.Val & 0x00FF));
	mrf24j40_set_long_reg( ram_addr++, ((frmCtrl.bits.Val & 0xFF00) >> 8));
	mrf24j40_set_long_reg( ram_addr++, g_mrf24j40_ctx.tx_seq++);
	if(pkt->header.flags.bits.broadcast == 0){
		if(frmCtrl.bits.intraPAN){
			mrf24j40_set_long_reg( ram_addr++, pkt->header.dest_pan_id[0]);
			mrf24j40_set_long_reg( ram_addr++, pkt->header.dest_pan_id[1]);
		}else{
			mrf24j40_set_long_reg( ram_addr++, g_mrf24j40_ctx.pan_id[0]);
			mrf24j40_set_long_reg( ram_addr++, g_mrf24j40_ctx.pan_id[1]);
		}
	}else{
		mrf24j40_set_long_reg( ram_addr++, 0xff);
		mrf24j40_set_long_reg( ram_addr++, 0xff);
	}
	// 8 byte
	if(pkt->header.flags.bits.broadcast){
		if(frmCtrl.bits.destAddrMode == mrf24j40_addrmode_16bit){
			mrf24j40_set_long_reg( ram_addr++, 0xFF);
			mrf24j40_set_long_reg( ram_addr++, 0xFF);
		}else{
			for(i = 0; i < 8 ; i++)
				mrf24j40_set_long_reg( ram_addr++, 0xFF);
		}
	}else{
		if(frmCtrl.bits.destAddrMode == mrf24j40_addrmode_16bit){
			mrf24j40_set_long_reg( ram_addr++, pkt->header.dest_addr[0]);
			mrf24j40_set_long_reg( ram_addr++, pkt->header.dest_addr[1]);
		}else{
			for(i = 0; i < 8 ; i++)
				mrf24j40_set_long_reg( ram_addr++, pkt->header.dest_addr[i]);
		}
	}
	if(frmCtrl.bits.srcAddrMode == mrf24j40_addrmode_16bit){
		mrf24j40_set_long_reg( ram_addr++, g_mrf24j40_ctx.short_address[0]);
		mrf24j40_set_long_reg( ram_addr++, g_mrf24j40_ctx.short_address[1]);
	}else{
		for(i = 0; i < 8; i++){
			mrf24j40_set_long_reg( ram_addr++, g_mrf24j40_ctx.long_address[i]);
		}
	}

	for(i = 0; i < pkt->data_len; i++){
		mrf24j40_set_long_reg( ram_addr++, pkt->data[i]);
	}
	// trigger tx
	if(frmCtrl.bits.ackReq == 1) i = 0x05;
	else i = 0x01;
	mrf24j40_set_short_reg(MRF24J40_WRITE_TXNMTRIG, i);
//	LREP("write to tx fifo done len %d\r\n", pkt->data_len);
}
static void mrf24j40_workqueue_handler(struct work_struct *w){
	int i;
	struct mrf24j40_write_node* pkt = 0;
	struct mrf24j40_read_node* read_node = 0;
	MRF24J40_IFREG	flags;
	u8 len;
	u8 u8val;
//	LREP("mrf24j40_workqueue_handler\r\n");
	flags.Val = mrf24j40_get_short_reg(MRF24J40_READ_ISRSTS);
	//LREP("flag=%02X\r\n", flags.Val);

	if(g_mrf24j40_ctx.mode == mrf24j40_mode_rx){
		spin_lock(&g_mrf24j40_ctx.lock);
		for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
			if(g_mrf24j40_ctx.write_list[i].flags & 0x01){
				g_mrf24j40_ctx.current_tx_index = i;
				pkt = &g_mrf24j40_ctx.write_list[i];
				break;
			}
		}
		spin_unlock(&g_mrf24j40_ctx.lock);
		if(pkt){
			g_mrf24j40_ctx.mode = mrf24j40_mode_tx;
			mrf24j40_write_packet(pkt);

			spin_lock(&g_mrf24j40_ctx.lock);
			pkt->flags &= ~((u8)1);
			for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
				if((g_mrf24j40_ctx.write_list[i].flags & 0x00) == 0){
					break;
				}
			}
			spin_unlock(&g_mrf24j40_ctx.lock);
			if(i < DRV_MRF24J40_WRITE_COUNT)
				wake_up_interruptible(&g_mrf24j40_ctx.wait_queue);
		}
	}

	if(flags.bits.RF_TXIF){
		g_mrf24j40_ctx.mode = mrf24j40_mode_rx;
		u8val = mrf24j40_get_short_reg(MRF24J40_READ_TXSR);

		spin_lock(&g_mrf24j40_ctx.lock);
		if(u8val & 0x01){
//			if(g_mrf24j40_ctx.current_tx_index >= 0
//					&& g_mrf24j40_ctx.current_tx_index < DRV_MRF24J40_WRITE_COUNT){
//				g_mrf24j40_ctx.write_list[g_mrf24j40_ctx.current_tx_index].flags |= 0x02;
//			}
			LREP_WARN("tx failed, retry count exceeded\r\n");
		}
		for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
			if(g_mrf24j40_ctx.write_list[i].flags & 0x01){
				pkt = &g_mrf24j40_ctx.write_list[i];
				g_mrf24j40_ctx.current_tx_index = i;
				break;
			}
		}
		spin_unlock(&g_mrf24j40_ctx.lock);
		if(pkt){
			g_mrf24j40_ctx.mode = mrf24j40_mode_tx;
			mrf24j40_write_packet(pkt);

			spin_lock(&g_mrf24j40_ctx.lock);
			pkt->flags &= ~((u8)1);

			for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
				if((g_mrf24j40_ctx.write_list[i].flags & 0x00) == 0){
					break;
				}
			}
			spin_unlock(&g_mrf24j40_ctx.lock);
			if(i < DRV_MRF24J40_WRITE_COUNT)
				wake_up_interruptible(&g_mrf24j40_ctx.wait_queue);
		}
	}else if(flags.bits.RF_RXIF){
		spin_lock(&g_mrf24j40_ctx.lock);
		for(i = 0; i < DRV_MRF24J40_READ_COUNT; i++){
			if((g_mrf24j40_ctx.read_list[i].flags & 0x00) == 0){
				read_node  = &g_mrf24j40_ctx.read_list[i];
				break;
			}
		}
		spin_unlock(&g_mrf24j40_ctx.lock);
		if(read_node){
			mrf24j40_set_short_reg(MRF24J40_WRITE_BBREG1, 0x40);	// Disable RX
			len = mrf24j40_get_long_reg(0x300) + 4;
			if(len > DRV_MRF24J40_READ_PAYLOAD_MAX_LENGTH)
				len = DRV_MRF24J40_READ_PAYLOAD_MAX_LENGTH;
			read_node->data[0] = len;
			for(i = 1 ;i < len; i++){
				read_node->data[i] = mrf24j40_get_long_reg(0x300 + i);
			}
			mrf24j40_set_short_reg(MRF24J40_WRITE_RXFLUSH, 0x01);
			mrf24j40_set_short_reg(MRF24J40_WRITE_BBREG1, 0x00);	// Enable RX
			spin_lock(&g_mrf24j40_ctx.lock);
			read_node->flags |= 0x01;
			spin_unlock(&g_mrf24j40_ctx.lock);
		}else{
			LREP_WARN("no more rx space\r\n");
		}
	}
	else if(flags.bits.SECIF){
		mrf24j40_set_short_reg(MRF24J40_WRITE_SECCR0, 0x80);
			//LREP("\r\nSECIF\r\n");
	}
}

static DECLARE_DELAYED_WORK(g_mrf24j40_action, mrf24j40_workqueue_handler);

static irqreturn_t mrf24j40_isr (int irq, void *dev_id, struct pt_regs *regs)
{
	//unsigned long flags;
	
	//local_irq_save(flags);
	
//	LREP("interrupt\r\n");
	queue_delayed_work(g_mrf24j40_ctx.wq, &g_mrf24j40_action, 0);
	//local_irq_restore(flags);
	return IRQ_HANDLED;
}

static int mrf24j40_open(struct inode *inode, struct file *filp)
{
	int i;
	if(g_mrf24j40_ctx.is_opened) return -1;

	//LREP("open");
	mrf24j40_initialize();

	for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
		g_mrf24j40_ctx.write_list[i].flags = 0x00;
	}
	for(i = 0; i < DRV_MRF24J40_READ_COUNT; i++){
		g_mrf24j40_ctx.read_list[i].flags = 0x00;
	}
	g_mrf24j40_ctx.mode = mrf24j40_mode_rx;

	g_mrf24j40_ctx.is_opened  = 1;
	return 0;
}
static int mrf24j40_release(struct inode *inode, struct file *filp)
{
	//LREP("close");
	g_mrf24j40_ctx.is_opened = 0;
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
static int mrf24j40_write_to_queue_packet(struct rf_mac_write_packet* pkt){
	int ret = -1;
	int i = 0;
	struct mrf24j40_write_node *node = 0;

	spin_lock(&g_mrf24j40_ctx.lock);
	for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
		if((g_mrf24j40_ctx.write_list[i].flags & 0x01) == 0){
			node = &g_mrf24j40_ctx.write_list[i];
			break;
		}
	}
	if(node){
		if(pkt->data_len > DRV_MRF24J40_WRITE_PAYLOAD_MAX_LENGTH)
			pkt->data_len = DRV_MRF24J40_WRITE_PAYLOAD_MAX_LENGTH;
		memcpy(&node->header, &pkt->header, sizeof(struct rf_mac_write_packet_header));
		memcpy(node->data, pkt->data, pkt->data_len);
		node->data_len = pkt->data_len;
		node->flags = 0x01;
		ret = 0;
	}else{
		LREP_WARN("no more tx space\r\n");
	}
	spin_unlock(&g_mrf24j40_ctx.lock);

	queue_delayed_work(g_mrf24j40_ctx.wq, &g_mrf24j40_action, 0);

	return ret;
}
static int mrf24j40_read_from_queue_packet(struct rf_mac_read_packet *pkt){
	int ret = 0;
	int i;
	u8 hdr_len, payload_len;
	struct mrf24j40_read_node *node = 0;
	struct mrf24j40_frm_ctrl *frmCtrl;

	spin_lock(&g_mrf24j40_ctx.lock);
	for(i = 0; i < DRV_MRF24J40_READ_COUNT; i++){
		if((g_mrf24j40_ctx.read_list[i].flags & 0x01) == 1){
			node = &g_mrf24j40_ctx.read_list[i];
			break;
		}
	}
	if(node){
		frmCtrl = (struct mrf24j40_frm_ctrl *)&node->data[1];	// offset hdr_len
		hdr_len = 1+2+1+2; // frame_len(1)+frameCtrl(2)+seq(1)+PANId(2)
		if(frmCtrl->bits.destAddrMode == mrf24j40_addrmode_16bit){
			pkt->header.flags.bits.dest_addr_64bit = 0;
			for(i = 0; i < 2; i++){
				pkt->header.dest_addr[i] = node->data[hdr_len++];
			}
		}
		else {
			pkt->header.flags.bits.dest_addr_64bit = 1;
			for(i = 0; i < 8; i++){
				pkt->header.dest_addr[i] = node->data[hdr_len++];
			}
		}

		if(frmCtrl->bits.srcAddrMode == mrf24j40_addrmode_16bit){
			pkt->header.flags.bits.src_addr_64bit = 0;
			for(i = 0; i < 2; i++){
				pkt->header.src_addr[i] = node->data[hdr_len++];
			}
		}
		else {
			pkt->header.flags.bits.src_addr_64bit = 1;
			for(i = 0; i < 8; i++){
				pkt->header.src_addr[i] = node->data[hdr_len++];
			}
		}

		payload_len = node->data_len - hdr_len - 4;
		if(payload_len < 0) payload_len = 0;
		else
			memcpy(pkt->data, &node->data[hdr_len], payload_len);
		pkt->data_len = payload_len;

		pkt->header.seq = node->data[3];
		for(i = 0; i < 2; i++)
			pkt->header.dest_pan_id[i] = node->data[4+i];
		pkt->header.flags.bits.ack_req = frmCtrl->bits.ackReq;
		pkt->header.flags.bits.intra_pan = frmCtrl->bits.intraPAN;
		pkt->header.lqi 	= node->data[node->data_len - 2];
		pkt->header.rssi 	= node->data[node->data_len - 1];

		node->flags = 0x00;
		ret = 0;
	}else{
		LREP_WARN("no more tx space\r\n");
	}
	spin_unlock(&g_mrf24j40_ctx.lock);

	return ret;
}
static long mrf24j40_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	long ret = -EINVAL;
	switch(cmd){
		case RF_MAC_IOC_WR_PACKET:{
//			LREP("write packet to queue\r\n");
			ret = mrf24j40_write_to_queue_packet((struct rf_mac_write_packet*)arg);
			break;
		}
		case RF_MAC_IOC_RD_PACKET:{
//			LREP("write packet to queue\r\n");
			ret = mrf24j40_read_from_queue_packet((struct rf_mac_read_packet*)arg);
			break;
		}
		case RF_MAC_IOC_WR_SHORT_ADDRESS:{
			mrf24j40_set_short_address((u8*)arg);
			ret = 0;
			break;
		}
		case RF_MAC_IOC_WR_LONG_ADDRESS:{
			mrf24j40_set_long_address((u8*)arg);
			ret = 0;
			break;
		}
		case RF_MAC_IOC_WR_PAN_ID:{
			mrf24j40_set_pan_id((u8*)arg);
			ret = 0;
			break;
		}
		default:
			LREP_WARN("unknown cmd %X\r\n", cmd);
	}
	return ret;
}
static unsigned int mrf24j40_poll(struct file *filp, struct poll_table_struct * wait)
{	
	int ret = 0;
	int i;
//	LREP("poll begin\r\n");
	spin_lock(&g_mrf24j40_ctx.lock);
	for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
		if((g_mrf24j40_ctx.write_list[i].flags & 0x01) == 0){
			ret |= POLLOUT;
			break;
		}
//		if((g_mrf24j40_ctx.write_list[i].flags & 0x02) == 1){
//			ret |= POLLERR;
//		}
	}
	for(i = 0; i < DRV_MRF24J40_READ_COUNT; i++){
		if((g_mrf24j40_ctx.read_list[i].flags & 0x01) == 1){
			ret |= POLLIN|POLLPRI;
			break;
		}
	}
	spin_unlock(&g_mrf24j40_ctx.lock);

	if(i >= DRV_MRF24J40_WRITE_COUNT){
		poll_wait(filp, &g_mrf24j40_ctx.wait_queue, wait);
		spin_lock(&g_mrf24j40_ctx.lock);
		for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
			if((g_mrf24j40_ctx.write_list[i].flags & 0x01) == 0){
				ret |= POLLOUT;
				break;
			}
//			if((g_mrf24j40_ctx.write_list[i].flags & 0x02) == 1){
//				ret |= POLLERR;
//			}
		}
		for(i = 0; i < DRV_MRF24J40_READ_COUNT; i++){
			if((g_mrf24j40_ctx.read_list[i].flags & 0x01) == 1){
				ret |= POLLIN|POLLPRI;
				break;
			}
		}
		spin_unlock(&g_mrf24j40_ctx.lock);
	}
//	LREP("poll end\r\n");
	return ret;
}
static int mrf24j40_spi_probe(struct spi_device *spi_device)
{
	//LREP("probe");
	g_mrf24j40_ctx.spi_device = spi_device;
	return 0;
}

static int mrf24j40_spi_remove(struct spi_device *spi_device)
{
	//LREP("remove");
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
	int irq, i;
	
	g_mrf24j40_ctx.c_dev 			= 0;
	g_mrf24j40_ctx.is_opened 		= 0;
	g_mrf24j40_ctx.tx_seq			= 0;
	for(i = 0; i < DRV_MRF24J40_WRITE_COUNT; i++){
		g_mrf24j40_ctx.write_list[i].flags = 0x00;
	}
	for(i = 0; i < DRV_MRF24J40_READ_COUNT; i++){
		g_mrf24j40_ctx.read_list[i].flags = 0x00;
	}
	g_mrf24j40_ctx.mode = mrf24j40_mode_rx;
	spin_lock_init(&g_mrf24j40_ctx.lock);
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
	spi_master = spi_busnum_to_master(g_mrf24j40_ctx.spi.spi_bus_num);	// SPI bus 0
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
	spi_device->chip_select = g_mrf24j40_ctx.spi.spi_chip_select;	// SPI chip select 0

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
		spi_device->max_speed_hz 	= 15000000;
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

	init_waitqueue_head(&g_mrf24j40_ctx.wait_queue);
	g_mrf24j40_ctx.wq = create_singlethread_workqueue("mrf24j40_wq");

	// gpio
	result = gpio_request_array(g_mrf24j40_ctx.gpios, ARRAY_SIZE(g_mrf24j40_ctx.gpios));
	if (result){
		LREP_WARN("gpio_request_array fail %d\r\n", result);
		goto INIT_FAIL_3;
	}
	irq = gpio_to_irq(g_mrf24j40_ctx.gpios[DRV_MRF24J40_GPIO_INTR].gpio);
	if(irq < 0){
		LREP_WARN("GPIO to IRQ mapping faiure\r\n");
		goto INIT_FAIL_4;
   }
   if (request_irq(irq, (irq_handler_t ) mrf24j40_isr, IRQF_TRIGGER_FALLING,
		   g_mrf24j40_ctx.gpios[DRV_MRF24J40_GPIO_INTR].label,
		   0)) {
	   LREP_WARN("irq request failure\r\n");
	   goto INIT_FAIL_4;
   }
   g_mrf24j40_ctx.intr_irq = irq;


	LREP("init done\r\n");
	result = 0;
	goto INIT_DONE;
INIT_FAIL_4:
	gpio_free_array(g_mrf24j40_ctx.gpios, ARRAY_SIZE(g_mrf24j40_ctx.gpios));
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
	cancel_delayed_work(&g_mrf24j40_action);
	destroy_workqueue(g_mrf24j40_ctx.wq);

	free_irq(g_mrf24j40_ctx.intr_irq, 0);
	gpio_free_array(g_mrf24j40_ctx.gpios, ARRAY_SIZE(g_mrf24j40_ctx.gpios));

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

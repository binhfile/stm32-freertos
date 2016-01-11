/*
 * mrf24j40_reg.h
 *
 *  Created on: Jan 11, 2016
 *      Author: dev
 */

#ifndef MRF24J40_MRF24J40_REG_H_
#define MRF24J40_MRF24J40_REG_H_
#include <linux/kernel.h>

struct __attribute__((packed)) mrf24j40_frm_ctrl{
    union{
        u16    Val;
        struct{
            u16 frmType    : 3;
            u16 secEn      : 1;
            u16 frmPending : 1;
            u16 ackReq     : 1;
            u16 intraPAN   : 1;        // msg in current PAN
            u16 reserver1   : 3;
            u16 destAddrMode: 2;
            u16 reserver2   : 2;
            u16 srcAddrMode : 2;
        };
    }bits;
};
enum mrf24j40_frmtype{
	mrf24j40_frmtype_data = 0x01,
	mrf24j40_frmtype_ack,
	mrf24j40_frmtype_cmd
};
enum mrf24j40_addrmode{
	mrf24j40_addrmode_16bit = 0x02,
	mrf24j40_addrmode_64bit = 0x03
};

typedef union
{
    u8 Val;               // value of interrupts
    struct
    {
        u8 RF_TXIF :1;    // transmission finish interrupt
        u8 reser1:2;
        u8 RF_RXIF :1;    // receiving a packet interrupt
        u8 SECIF :1;      // receiving a secured packet interrupt
        u8 reser2:4;
    }bits;                  // bit map of interrupts
} MRF24J40_IFREG;

typedef union
{
    u8 Val;
    struct
    {
        u8        TX_BUSY             : 1;
        u8        TX_PENDING_ACK      : 1;
        u8        TX_FAIL             : 1;
        u8        RX_SECURITY         : 1;
        u8        RX_IGNORE_SECURITY  : 1;
        u8        RX_BUFFERED         : 1;
        u8        SEC_IF              : 1;
    } bits;
} MRF24J40_STATUS;


#define MRF24J40_SEC_LEVEL_CBC_MAC_32    7
#define MRF24J40_SEC_LEVEL_CBC_MAC_64    6
#define MRF24J40_SEC_LEVEL_CBC_MAC_128   5
#define MRF24J40_SEC_LEVEL_CCM_32        4
#define MRF24J40_SEC_LEVEL_CCM_64        3
#define MRF24J40_SEC_LEVEL_CCM_128       2
#define MRF24J40_SEC_LEVEL_CTR           1

//long address registers
#define MRF24J40_RFCTRL0 (0x200)
#define MRF24J40_RFCTRL1 (0x201)
#define MRF24J40_RFCTRL2 (0x202)
#define MRF24J40_RFCTRL3 (0x203)
#define MRF24J40_RFCTRL4 (0x204)
#define MRF24J40_RFCTRL5 (0x205)
#define MRF24J40_RFCTRL6 (0x206)
#define MRF24J40_RFCTRL7 (0x207)
#define MRF24J40_RFCTRL8 (0x208)
#define MRF24J40_CAL1 (0x209)
#define MRF24J40_CAL2 (0x20a)
#define MRF24J40_CAL3 (0x20b)
#define MRF24J40_SFCNTRH (0x20c)
#define MRF24J40_SFCNTRM (0x20d)
#define MRF24J40_SFCNTRL (0x20e)
#define MRF24J40_RFSTATE (0x20f)
#define MRF24J40_RSSI (0x210)
#define MRF24J40_CLKIRQCR (0x211)
#define MRF24J40_SRCADRMODE (0x212)
#define MRF24J40_SRCADDR0 (0x213)
#define MRF24J40_SRCADDR1 (0x214)
#define MRF24J40_SRCADDR2 (0x215)
#define MRF24J40_SRCADDR3 (0x216)
#define MRF24J40_SRCADDR4 (0x217)
#define MRF24J40_SRCADDR5 (0x218)
#define MRF24J40_SRCADDR6 (0x219)
#define MRF24J40_SRCADDR7 (0x21a)
#define MRF24J40_RXFRAMESTATE (0x21b)
#define MRF24J40_SECSTATUS (0x21c)
#define MRF24J40_STCCMP (0x21d)
#define MRF24J40_HLEN (0x21e)
#define MRF24J40_FLEN (0x21f)
#define MRF24J40_SCLKDIV (0x220)
//#define reserved (0x221)
#define MRF24J40_WAKETIMEL (0x222)
#define MRF24J40_WAKETIMEH (0x223)
#define MRF24J40_TXREMCNTL (0x224)
#define MRF24J40_TXREMCNTH (0x225)
#define MRF24J40_TXMAINCNTL (0x226)
#define MRF24J40_TXMAINCNTM (0x227)
#define MRF24J40_TXMAINCNTH0 (0x228)
#define MRF24J40_TXMAINCNTH1 (0x229)
#define MRF24J40_RFMANUALCTRLEN (0x22a)
#define MRF24J40_RFMANUALCTRL (0x22b)
#define MRF24J40_RFRXCTRL RFMANUALCTRL
#define MRF24J40_TxDACMANUALCTRL (0x22c)
#define MRF24J40_RFMANUALCTRL2 (0x22d)
#define MRF24J40_TESTRSSI (0x22e)
#define MRF24J40_TESTMODE (0x22f)

#define MRF24J40_NORMAL_TX_FIFO  (0x000)
#define MRF24J40_BEACON_TX_FIFO  (0x080)
#define MRF24J40_GTS1_TX_FIFO    (0x100)
#define MRF24J40_GTS2_TX_FIFO    (0x180)

#define MRF24J40_RX_FIFO         (0x300)

#define MRF24J40_SECURITY_FIFO   (0x280)





//short address registers for reading
#define MRF24J40_READ_RXMCR (0x00)
#define MRF24J40_READ_PANIDL (0x02)
#define MRF24J40_READ_PANIDH (0x04)
#define MRF24J40_READ_SADRL (0x06)
#define MRF24J40_READ_SADRH (0x08)
#define MRF24J40_READ_EADR0 (0x0A)
#define MRF24J40_READ_EADR1 (0x0C)
#define MRF24J40_READ_EADR2 (0x0E)
#define MRF24J40_READ_EADR3 (0x10)
#define MRF24J40_READ_EADR4 (0x12)
#define MRF24J40_READ_EADR5 (0x14)
#define MRF24J40_READ_EADR6 (0x16)
#define MRF24J40_READ_EADR7 (0x18)
#define MRF24J40_READ_RXFLUSH (0x1a)
#define MRF24J40_READ_TXSTATE0 (0x1c)
#define MRF24J40_READ_TXSTATE1 (0x1e)
#define MRF24J40_READ_ORDER (0x20)
#define MRF24J40_READ_TXMCR (0x22)
#define MRF24J40_READ_ACKTMOUT (0x24)
#define MRF24J40_READ_SLALLOC (0x26)
#define MRF24J40_READ_SYMTICKL (0x28)
#define MRF24J40_READ_SYMTICKH (0x2A)
#define MRF24J40_READ_PAONTIME (0x2C)
#define MRF24J40_READ_PAONSETUP (0x2E)
#define MRF24J40_READ_FFOEN (0x30)
#define MRF24J40_READ_CSMACR (0x32)
#define MRF24J40_READ_TXBCNTRIG (0x34)
#define MRF24J40_READ_TXNMTRIG (0x36)
#define MRF24J40_READ_TXG1TRIG (0x38)
#define MRF24J40_READ_TXG2TRIG (0x3A)
#define MRF24J40_READ_ESLOTG23 (0x3C)
#define MRF24J40_READ_ESLOTG45 (0x3E)
#define MRF24J40_READ_ESLOTG67 (0x40)
#define MRF24J40_READ_TXPEND (0x42)
#define MRF24J40_READ_TXBCNINTL (0x44)
#define MRF24J40_READ_FRMOFFSET (0x46)
#define MRF24J40_READ_TXSR (0x48)
#define MRF24J40_READ_TXLERR (0x4A)
#define MRF24J40_READ_GATE_CLK (0x4C)
#define MRF24J40_READ_TXOFFSET (0x4E)
#define MRF24J40_READ_HSYMTMR0 (0x50)
#define MRF24J40_READ_HSYMTMR1 (0x52)
#define MRF24J40_READ_SOFTRST (0x54)
#define MRF24J40_READ_BISTCR (0x56)
#define MRF24J40_READ_SECCR0 (0x58)
#define MRF24J40_READ_SECCR1 (0x5A)
#define MRF24J40_READ_TXPEMISP (0x5C)
#define MRF24J40_READ_SECISR (0x5E)
#define MRF24J40_READ_RXSR (0x60)
#define MRF24J40_READ_ISRSTS (0x62)
#define MRF24J40_READ_INTMSK (0x64)
#define MRF24J40_READ_GPIO (0x66)
#define MRF24J40_READ_GPIODIR (0x68)
#define MRF24J40_READ_SLPACK (0x6A)
#define MRF24J40_READ_RFCTL (0x6C)
#define MRF24J40_READ_SECCR2 (0x6E)
#define MRF24J40_READ_BBREG0 (0x70)
#define MRF24J40_READ_BBREG1 (0x72)
#define MRF24J40_READ_BBREG2 (0x74)
#define MRF24J40_READ_BBREG3 (0x76)
#define MRF24J40_READ_BBREG4 (0x78)
#define MRF24J40_READ_BBREG5 (0x7A)
#define MRF24J40_READ_BBREG6 (0x7C)
#define MRF24J40_READ_RSSITHCCA (0x7E)

//short address registers for writing
//short address registers for reading
#define MRF24J40_WRITE_RXMCR (0x01)
#define MRF24J40_WRITE_PANIDL (0x03)
#define MRF24J40_WRITE_PANIDH (0x05)
#define MRF24J40_WRITE_SADRL (0x07)
#define MRF24J40_WRITE_SADRH (0x09)
#define MRF24J40_WRITE_EADR0 (0x0B)
#define MRF24J40_WRITE_EADR1 (0x0D)
#define MRF24J40_WRITE_EADR2 (0x0F)
#define MRF24J40_WRITE_EADR3 (0x11)
#define MRF24J40_WRITE_EADR4 (0x13)
#define MRF24J40_WRITE_EADR5 (0x15)
#define MRF24J40_WRITE_EADR6 (0x17)
#define MRF24J40_WRITE_EADR7 (0x19)
#define MRF24J40_WRITE_RXFLUSH (0x1B)
#define MRF24J40_WRITE_TXSTATE0 (0x1D)
#define MRF24J40_WRITE_TXSTATE1 (0x1F)
#define MRF24J40_WRITE_ORDER (0x21)
#define MRF24J40_WRITE_TXMCR (0x23)
#define MRF24J40_WRITE_ACKTMOUT (0x25)
#define MRF24J40_WRITE_SLALLOC (0x27)
#define MRF24J40_WRITE_SYMTICKL (0x29)
#define MRF24J40_WRITE_SYMTICKH (0x2B)
#define MRF24J40_WRITE_PAONTIME (0x2D)
#define MRF24J40_WRITE_PAONSETUP (0x2F)
#define MRF24J40_WRITE_FFOEN (0x31)
#define MRF24J40_WRITE_CSMACR (0x33)
#define MRF24J40_WRITE_TXBCNTRIG (0x35)
#define MRF24J40_WRITE_TXNMTRIG (0x37)
#define MRF24J40_WRITE_TXG1TRIG (0x39)
#define MRF24J40_WRITE_TXG2TRIG (0x3B)
#define MRF24J40_WRITE_ESLOTG23 (0x3D)
#define MRF24J40_WRITE_ESLOTG45 (0x3F)
#define MRF24J40_WRITE_ESLOTG67 (0x41)
#define MRF24J40_WRITE_TXPEND (0x43)
#define MRF24J40_WRITE_TXBCNINTL (0x45)
#define MRF24J40_WRITE_FRMOFFSET (0x47)
#define MRF24J40_WRITE_TXSR (0x49)
#define MRF24J40_WRITE_TXLERR (0x4B)
#define MRF24J40_WRITE_GATE_CLK (0x4D)
#define MRF24J40_WRITE_TXOFFSET (0x4F)
#define MRF24J40_WRITE_HSYMTMR0 (0x51)
#define MRF24J40_WRITE_HSYMTMR1 (0x53)
#define MRF24J40_WRITE_SOFTRST (0x55)
#define MRF24J40_WRITE_BISTCR (0x57)
#define MRF24J40_WRITE_SECCR0 (0x59)
#define MRF24J40_WRITE_SECCR1 (0x5B)
#define MRF24J40_WRITE_TXPEMISP (0x5D)
#define MRF24J40_WRITE_SECISR (0x5F)
#define MRF24J40_WRITE_RXSR (0x61)
#define MRF24J40_WRITE_ISRSTS (0x63)
#define MRF24J40_WRITE_INTMSK (0x65)
#define MRF24J40_WRITE_GPIO (0x67)
#define MRF24J40_WRITE_GPIODIR (0x69)
#define MRF24J40_WRITE_SLPACK (0x6B)
#define MRF24J40_WRITE_RFCTL (0x6D)
#define MRF24J40_WRITE_SECCR2 (0x6F)
#define MRF24J40_WRITE_BBREG0 (0x71)
#define MRF24J40_WRITE_BBREG1 (0x73)
#define MRF24J40_WRITE_BBREG2 (0x75)
#define MRF24J40_WRITE_BBREG3 (0x77)
#define MRF24J40_WRITE_BBREG4 (0x79)
#define MRF24J40_WRITE_BBREG5 (0x7B)
#define MRF24J40_WRITE_BBREG6 (0x7D)
#define MRF24J40_WRITE_RSSITHCCA (0x7F)

#define MRF24J40_CHANNEL_11 0x00
#define MRF24J40_CHANNEL_12 0x10
#define MRF24J40_CHANNEL_13 0x20
#define MRF24J40_CHANNEL_14 0x30
#define MRF24J40_CHANNEL_15 0x40
#define MRF24J40_CHANNEL_16 0x50
#define MRF24J40_CHANNEL_17 0x60
#define MRF24J40_CHANNEL_18 0x70
#define MRF24J40_CHANNEL_19 0x80
#define MRF24J40_CHANNEL_20 0x90
#define MRF24J40_CHANNEL_21 0xa0
#define MRF24J40_CHANNEL_22 0xb0
#define MRF24J40_CHANNEL_23 0xc0
#define MRF24J40_CHANNEL_24 0xd0
#define MRF24J40_CHANNEL_25 0xe0
#define MRF24J40_CHANNEL_26 0xf0

#define FULL_CHANNEL_MAP        0x07FFF800



#endif /* MRF24J40_MRF24J40_REG_H_ */

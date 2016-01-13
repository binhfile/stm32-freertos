#ifndef AT93C_H__
#define AT93C_H__

#define AT93C_IOC_WR_ERASE_ALL	0x01

#define AT93C_IOC_RD_SIZE       (0x08|0x02) // args = &unsigned int
#define AT93C_IOC_RD_NAME       (0x08|0x03) // args = &char[32]

#endif

#ifndef __RF_MAC__
#define __RF_MAC__

#define RF_MAC_IOC_WR_PACKET		0x01	// args = &struct rf_mac_write_packet
#define RF_MAC_IOC_WR_SHORT_ADDRESS	0x02	// args = u8[2]
#define RF_MAC_IOC_WR_LONG_ADDRESS	0x03	// args = u8[8]
#define RF_MAC_IOC_WR_PAN_ID		0x04	// args = u8[2]

#define RF_MAC_IOC_RD_PACKET		(0x08|RF_MAC_IOC_WR_PACKET)	// args = &struct rf_mac_read_packet


struct rf_mac_write_packet_header{
	unsigned char dest_pan_id[2];
	unsigned char dest_addr[8];
	union{
		unsigned char Val;
		struct{
			unsigned char broadcast : 1;
			unsigned char ack_req	: 1;
			unsigned char intra_pan	: 1;
			unsigned char src_addr_64bit: 1;
			unsigned char dest_addr_64bit: 1;
			unsigned char reserver  : 3;
		}bits;
	}flags;

};
struct rf_mac_write_packet{
	struct rf_mac_write_packet_header header;
	unsigned char data_len;
	unsigned char data[1];
};

struct rf_mac_read_packet_header{
	unsigned char dest_pan_id[2];
	unsigned char dest_addr[8];
	unsigned char src_addr[8];
	unsigned char seq;
	unsigned char rssi;
	unsigned char lqi;
	union{
		unsigned char Val;
		struct{
			unsigned char broadcast : 1;
			unsigned char ack_req	: 1;
			unsigned char intra_pan	: 1;
			unsigned char src_addr_64bit: 1;
			unsigned char dest_addr_64bit: 1;
			unsigned char reserver  : 3;
		}bits;
	}flags;

};
struct rf_mac_read_packet{
	struct rf_mac_read_packet_header header;
	unsigned char data_len;
	unsigned char data[1];
};

#endif

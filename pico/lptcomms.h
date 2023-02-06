#ifndef _LPTCOMMS_H
#define _LPTCOMMS_H

#define LPTS_HELLO            0x00
#define LPTS_QUERY_DISKS      0x01
#define LPTS_QUERY_DISK_CAPS  0x02
#define LPTS_READ_BLOCK       0x03
#define LPTS_WRITE_BLOCK      0x04
        
#define LPTS_VERSION_MAJOR    0
#define LPTS_VERSION_MINOR    1


int lptcomms_init();
void lptcomms_task();

int lptcomms_readlpt();
int lptcomms_writelpt(uint8_t b);
int lptcomms_hasbyte();

int lptcomms_readnybble();
int lptcomms_readbytes(uint8_t *buff, int len);
int lptcomms_readbyte();

int lptcomms_writebyte(uint8_t b);
int lptcomms_writenybble(uint8_t b);
int lptcomms_writebytes(uint8_t *d, int len);

#endif


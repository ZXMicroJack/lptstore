#ifndef _LPTCOMMS_H
#define _LPTCOMMS_H

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


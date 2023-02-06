#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <pico/time.h>

#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "hardware/flash.h"

#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"

#include "lptcomms.h"

#ifdef DEBUG
#define debug(a) printf a
#else
#define debug(a)
#endif

#ifdef SDEBUG
#define sdebug(a) printf a
#else
#define sdebug(a)
#endif

#if 0
int readlpt() {
  uint32_t dw = gpio_get_all() >> 20;
  uint8_t b = (dw >> 8) | ((dw&0x80) >> 6) | ((dw&0x40) >> 4) | ((dw&0x04) << 1) | ((dw&0x02) << 3);
  return b;
}

int hasbyte() {
  return ((readlpt() & 0x10) == 0x10);
}

#define PROTOCOL_TIMEOUT 200000

int waittimeout(int mask, int value) {
  int c;
  uint64_t time_now;
  
  time_now = time_us_64();
  while (((c = readlpt()) & mask) == value && time_us_64() < (time_now + PROTOCOL_TIMEOUT))
    tight_loop_contents();
  
  return (c & mask) == value ? -1 : c;
 }

int readnybble() {
  int c,d;
  
  // wait for host to signal 1 nybble ready, acknowledge 1
//   while (((c = readlpt()) & 0x10) == 0x00)
//     tight_loop_contents();
  c = waittimeout(0x10, 0x00);
  if (c < 0) {
    gpio_put(19, 0);
    return -1;
  }
  d = readlpt() & 0xf;
  gpio_put(19, 1);

//   while (((c = readlpt()) & 0x10) != 0x00)
//     tight_loop_contents();
  c = waittimeout(0x10, 0x10);
  if (c < 0) {
    return -1;
  }

  gpio_put(19, 0);
//   printf("[r:%x]", d);
  
  return d;
}


int readbyte() {
  uint8_t b;
  int i;
  
  i = readnybble();
  if (i < 0) {
    return -1;
  }
  b = i << 4;

  i = readnybble();
  if (i < 0) {
    return -1;
  }
  b |= i;
  return b;
}

int readbytes(uint8_t *buff, int len) {
  int i, r;
  for (i=0; i<len; i++) {
    r = readbyte();
    if (r < 0) return -1;
    buff[i] = r;
  }
  return 0;
}

int writenybble(uint8_t b) {
  int c,d;
  
  gpio_put(16, (b & 0x1) ? 0 : 1);
  gpio_put(17, (b & 0x2) ? 0 : 1);
  gpio_put(18, (b & 0x4) ? 0 : 1);
  gpio_put(20, (b & 0x8) ? 0 : 1);

  gpio_put(19, 1); // write upper nybble then raise signal
  c = waittimeout(0x10, 0x00);
  if (c < 0) {
    gpio_put(19, 0);
    return -1;
  }
//   while ((readlpt() & 0x10) == 0x00) // wait for ack
//     tight_loop_contents();

  gpio_put(19, 0); // write upper nybble then raise signal
//   while ((readlpt() & 0x10) != 0x00) // wait for ack
//     tight_loop_contents();
  c = waittimeout(0x10, 0x10);
  if (c < 0) {
    gpio_put(19, 0);
    return -1;
  }

  return 0;
}


int writebyte(uint8_t b) {
  if (writenybble(b >> 4) < 0) return -1;
  if (writenybble(b & 0xf) < 0) return -1;
  return 0;
}

int writebytes(uint8_t *d, int len) {
  int i;
  for (i=0; i<len; i++) {
    if (writebyte(d[i]) < 0) {
      return -1;
    }
  }
  return 0;
}
#endif

int init_disk(void) {
  return 0;
}
uint32_t get_disks(void) { return 0xffff; }
uint32_t get_disk_blocks(uint8_t disk) {
  return 32*1024*2;
}

#include "../dos/SDPP10/block0.h"
#include "../dos/SDPP10/block4.h"

#define MAX_BLOCKS 200

uint8_t blocks[MAX_BLOCKS][512];
uint16_t blockNr[MAX_BLOCKS];
uint8_t blockUnit[MAX_BLOCKS];
int nrBlocks = 0;


int read_disk_sector(uint8_t disk, int lbn, uint8_t *buff) {
  int j;
  
  for (j=0; j<nrBlocks; j++) {
    if (blockUnit[j] == disk && blockNr[j] == lbn) {
      memcpy(buff, blocks[j], 512);
      printf("read_disk_sector: getting block %d:%d from cache at position %d\n", disk, lbn, j);
      break;
    }
  }
  
  if (j>=nrBlocks) {
    if (lbn == 0) memcpy(buff, xaa, 512);
    else if (lbn == 4) memcpy(buff, xae, 512);
    else if (lbn == 68) memcpy(buff, xae, 512);
    else memset(buff, 0, 512);
  }
  return 0;
}

int write_disk_sector(uint8_t disk, int lbn, uint8_t *buff) {
  int j;
  for (j=0; j<nrBlocks; j++) {
    if (blockUnit[j] == disk && blockNr[j] == lbn) {
      memcpy(blocks[j], buff, 512);
      break;
    }
  }

  if (j>=nrBlocks && nrBlocks < MAX_BLOCKS) {
    memcpy(blocks[nrBlocks], buff, 512);
    blockUnit[nrBlocks] = disk;
    blockNr[nrBlocks++] = lbn;
    printf("write_disk_sector: Storing block %d:%d (%d)\n", disk, lbn, nrBlocks);
  }
  return 0;
}

int main()
{
  stdio_init_all();
  sleep_ms(1000); // usb settle delay
  
// #if PICO_NO_FLASH
//   enable_xip();
// #endif
  
//   uart_init (uart1, 31250);
//   uart_set_fifo_enabled(uart1, true);
//   gpio_set_function(4, GPIO_FUNC_UART);
//   gpio_set_function(5, GPIO_FUNC_UART);

  // set up error led
//   gpio_init(PICO_DEFAULT_LED_PIN);
//   gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  
  lptcomms_init();

#if 0
  int lut[] = {21,22,26,27,28};
  for (int i=0; i<(sizeof lut / sizeof lut[0]); i++) {
    gpio_init(lut[i]);
//     gpio_pull_up(i);
    gpio_set_dir(lut[i], GPIO_IN);
//     gpio_set_function(i, GPIO_FUNC_SIO);
  }

  int lut2[] = {16,17,18,19,20};
  for (int i=0; i<(sizeof lut2 / sizeof lut2[0]); i++) {
    gpio_init(lut2[i]);
//     gpio_pull_up(i);
    gpio_set_dir(lut2[i], GPIO_OUT);
    gpio_set_drive_strength(lut2[i], GPIO_DRIVE_STRENGTH_12MA);
    gpio_pull_up(lut2[i]);
//     gpio_set_function(i, GPIO_FUNC_SIO);
  }
#endif

//   gpio_init(PICO_DEFAULT_LED_PIN);
//   
//   uint64_t started;
  
//   muteAudio = 1;
//   multicore_reset_core1();
//   multicore_launch_core1(audio_core);

//   }
  int w = 0;
  uint8_t b = 0x01;
#if 0
  int s[5] = {0};
#endif
  
  for(;;) {
    int c = getchar_timeout_us(0);
    if (c == 'h') printf("(h)elp (q)uit (r)eadgpios (w)read (e)mitbyte (d)readgpios (f)writegpios\n");
    if (c == 'q') break;
    if (c == 'r') {
      uint32_t dw = gpio_get_all() >> 20;
      uint8_t b = (dw >> 8) | ((dw&0x80) >> 6) | ((dw&0x40) >> 4) | ((dw&0x04) << 1) | ((dw&0x02) << 3);

      printf("gpios: %08x (%02x)\n", (gpio_get_all() >> 20), b);
      
    }
    
    if (c == 'w') {
      printf("Reading: %02X\n", lptcomms_readbyte());
    }
    if (c == 'e') {
      printf("Emit: %02X\n", b);
      lptcomms_writebyte(b);
      b <<= 1;
      if (!b) b = 1;
    }
    
    if (c == 'd') {
      printf("in:%02X\n", lptcomms_readlpt());
    }
    
    if (c == 'f') {
      printf("f:%02X\n", b);
      lptcomms_writelpt(b);
      b <<= 1;
      if (b == 0x20) b = 1;
    }
    
    if (c == 'p') {
      // protocol mode
      int quit = 0;
      printf("Entering protocol mode.\n");
      while (!quit) {
        uint8_t buff[512];
#define LPTS_HELLO            0x00
#define LPTS_QUERY_DISKS      0x01
#define LPTS_QUERY_DISK_CAPS  0x02
#define LPTS_READ_BLOCK       0x03
#define LPTS_WRITE_BLOCK      0x04
        
#define LPTS_VERSION_MAJOR    0
#define LPTS_VERSION_MINOR    1

        int c = getchar_timeout_us(0);
        if (c == 'q') break;
        if (lptcomms_hasbyte()) {
          uint8_t cmd = lptcomms_readbyte();
          if (cmd != 0xff) {
            printf("cmd:%02X\n", cmd);
            switch(cmd) {
              case LPTS_HELLO:
                buff[0] = LPTS_VERSION_MAJOR;
                buff[1] = LPTS_VERSION_MINOR;
                lptcomms_writebytes(buff, 2);
                break;
              case LPTS_QUERY_DISKS: {
                uint32_t w = get_disks();
                buff[0] = w >> 24;
                buff[1] = (w >> 16) & 0xff;
                buff[2] = (w >> 8) & 0xff;
                buff[3] = w  & 0xff;
                lptcomms_writebytes(buff, 4);
                break;
              }
              case LPTS_QUERY_DISK_CAPS: {
                uint32_t w;
                if (lptcomms_readbytes(buff, 1) < 0) 
                  break;
                w = get_disk_blocks(buff[0]);
                buff[0] = w >> 24;
                buff[1] = (w >> 16) & 0xff;
                buff[2] = (w >> 8) & 0xff;
                buff[3] = w  & 0xff;
                // this will timeout and break anyway.
                lptcomms_writebytes(buff, 4);
                break;
              }
              case LPTS_READ_BLOCK: {
                if (lptcomms_readbytes(buff, 5) < 0)
                  break;
                
                uint8_t drv = buff[0];
                uint32_t lba = (buff[1] << 24) | (buff[2] << 16) | (buff[3] << 8) | buff[4];
                uint8_t r = read_disk_sector(drv, lba, buff);
                
                if (lptcomms_writebyte(r) < 0) break;
                if (!r) {
                  // this will timeout and break anyway.
                  lptcomms_writebytes(buff, 512);
                }
                break;
              }
              case LPTS_WRITE_BLOCK: {
                if (lptcomms_readbytes(buff, 5) < 0) break;
                
                uint8_t drv = buff[0];
                uint32_t lba = (buff[1] << 24) | (buff[2] << 16) | (buff[3] << 8) | buff[4];
                
                if (lptcomms_readbytes(buff, 512) < 0) break;
                uint8_t r = write_disk_sector(drv, lba, buff);
                
                // this will timeout and break anyway.
                lptcomms_writebyte(r); 
                break;
              }
                
            } // switch
          } // if
        } // if hasbyte
      }
      printf("Leaving protocol mode.\n");
    }
  }
  
// 	wtsynth_Kill();
  reset_usb_boot(0, 0);
	return 0;
}


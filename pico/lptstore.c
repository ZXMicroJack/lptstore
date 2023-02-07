#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <pico/time.h>

#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
// #include "hardware/flash.h"

#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"

#include "lptcomms.h"
#include "disk.h"
#include "lptprotocol.h"

#include "bsp/board.h"
#include "tusb.h"

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



void lptcomms_core() {
  lptcomms_init();
  disk_init();

  int w = 0;
  uint8_t b = 0x01;
  
  for(;;) {
#if 0
    int c = getchar_timeout_us(0);
    if (c == 'h') printf("(h)elp (q)uit (w)read (e)mitbyte (d)readgpios (f)writegpios\n");
    if (c == 'q') break;
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
        int c = getchar_timeout_us(0);
        if (c == 'q') break;

        lptprotocol_task();
      }
      printf("Leaving protocol mode.\n");
    }
#endif  
    lptprotocol_task();
    msc_task();
  }
}

uint8_t sector[512];
uint8_t sector2[512];

// extern int read_sector(int pdrv, uint8_t *buff, uint32_t sector);
// extern int write_sector(int pdrv, uint8_t *buff, uint32_t sector);
// 
// int read_sector(int sector, uint8_t *buff);
// int write_sector(int sector, uint8_t *buff);

extern void msc_task();

int main()
{
  stdio_init_all();
  uart_init (uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);

  board_init();
  tusb_init();
  
//   sleep_ms(1000); // usb settle delay
  printf("Starting up\n");
  
  multicore_reset_core1();
  multicore_launch_core1(lptcomms_core);

  
// int disk_write_sector(uint8_t disk, int lbn, uint8_t *buff);
// int disk_read_sector(uint8_t disk, int lbn, uint8_t *buff);
// uint32_t disk_get_blocks(uint8_t disk);
// uint32_t disk_get_drives(void);
// int disk_init(void);
  
  printf("Starting up 222\n");
  
  while (1)
  {
    // tinyusb host task
    tuh_task();
//     led_blinking_task();
    
#if 0
    switch(getchar_timeout_us(0)) {
      case 'r': {
        printf("read: returns %d\r\n", disk_read_sector(0, 0, sector));
        break;
      }
      case 'R': {
        printf("read2: returns %d\r\n", disk_read_sector(0, 0, sector2));
        break;
      }
      case 'w': {
        printf("write: returns %d\r\n", disk_write_sector(0, 0, sector));
        break;
      }
      case 'W': {
        printf("write2: returns %d\r\n", disk_write_sector(0, 0, sector2));
        break;
      }
      case 'd': {
        int i;
        for (i=0; i<512; i++) {
          if ((i&15) == 0) printf("\r\n");
          printf("%02X ", sector[i]);
        }
        break;
      }
      case 'D': {
        int i;
        for (i=0; i<512; i++) {
          if ((i&15) == 0) printf("\r\n");
          printf("%02X ", sector2[i]);
        }
        break;
      }
      case 'b': {
        memset(sector, 0, sizeof sector);
        printf("cleared\n");
        break;
      }
      case 'B': {
        memset(sector2, 0, sizeof sector2);
        printf("cleared2\n");
        break;
      }
    }
#endif

  }
  
  reset_usb_boot(0, 0);
	return 0;
}


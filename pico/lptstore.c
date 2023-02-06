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
#include "disk.h"

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
  disk_init();

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
                uint32_t w = disk_get_drives();
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
                w = disk_get_blocks(buff[0]);
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
                uint8_t r = disk_read_sector(drv, lba, buff);
                
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
                uint8_t r = disk_write_sector(drv, lba, buff);
                
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


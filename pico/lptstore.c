#include <stdio.h>
#include <stdint.h>
#include <pico/time.h>

#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "hardware/flash.h"

#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"

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

int readlpt() {
  uint32_t dw = gpio_get_all() >> 20;
  uint8_t b = (dw >> 8) | ((dw&0x80) >> 6) | ((dw&0x40) >> 4) | ((dw&0x04) << 1) | ((dw&0x02) << 3);
  return b;
}

int hasbyte() {
  return ((readlpt() & 0x10) == 0x10);
}

int readbyte() {
  int c, d;
//   printf("a");
  while (((c = readlpt()) & 0x10) == 0x00)
    tight_loop_contents();
  d = (c & 0xf) << 4;
//   printf("b");
  gpio_put(19, 1);
//   printf("c");
  while (((c = readlpt()) & 0x10) == 0x10)
    tight_loop_contents();
//   printf("d");
  d |= (c & 0xf);
  gpio_put(19, 0);

  // extra handshake
  while (((c = readlpt()) & 0x10) == 0x00)
    tight_loop_contents();
  gpio_put(19, 1);
  while (((c = readlpt()) & 0x10) == 0x10)
    tight_loop_contents();
  gpio_put(19, 0);
  


  // if this is removed then the PC doesn't seem to register the last
  // port waggle.
//   sleep_ms(2);
  
  
//   while (((c = readlpt()) & 0x10) == 0x00)
//     tight_loop_contents();
//   while (((c = readlpt()) & 0x10) == 0x10)
//     tight_loop_contents();

  //   printf("e");
  return d;
}

int readbytes(uint8_t *buff, int len) {
  int i;
  for (i=0; i<len; i++) {
    buff[i] = readbyte();
  }
  return 0;
}

void writebyte(uint8_t b) {
  gpio_put(16, (b & 0x10) ? 0 : 1);
  gpio_put(17, (b & 0x20) ? 0 : 1);
  gpio_put(18, (b & 0x40) ? 0 : 1);
  gpio_put(20, (b & 0x80) ? 0 : 1);
  gpio_put(19, 1);
  while ((readlpt() & 0x10) == 0x00)
    tight_loop_contents();
  gpio_put(16, (b & 0x01) ? 0 : 1);
  gpio_put(17, (b & 0x02) ? 0 : 1);
  gpio_put(18, (b & 0x04) ? 0 : 1);
  gpio_put(20, (b & 0x08) ? 0 : 1);
  gpio_put(19, 0);
  while ((readlpt() & 0x10) == 0x10)
    tight_loop_contents();
}

void writebytes(uint8_t *d, int len) {
  int i;
  for (i=0; i<len; i++) {
    writebyte(d[i]);
  }
}

int init_disk(void) {
  return 0;
}
uint32_t get_disks(void) { return 0xffff; }
uint32_t get_disk_blocks(uint8_t disk) {
  return 32*1024*2;
}

int read_disk_sector(uint8_t disk, int sector, uint8_t *buff) {
  return 0;
}

int write_disk_sector(uint8_t disk, int sector, uint8_t *buff) {
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
      printf("Reading: %02X\n", readbyte());
    }
    if (c == 'e') {
      printf("Emit: %02X\n", b);
      writebyte(b);
      b <<= 1;
      if (!b) b = 1;
    }
    
    if (c == 'd') {
      printf("in:%02X\n", readlpt());
    }
    
    if (c == 'f') {
      printf("f:%02X\n", b);
      gpio_put(16, (b & 0x01) ? 0 : 1);
      gpio_put(17, (b & 0x02) ? 0 : 1);
      gpio_put(18, (b & 0x04) ? 0 : 1);
      gpio_put(20, (b & 0x08) ? 0 : 1);
      gpio_put(19, (b & 0x10) ? 1 : 0);
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
        if (hasbyte()) {
          uint8_t cmd = readbyte();
          switch(cmd) {
            case LPTS_HELLO:
              buff[0] = LPTS_VERSION_MAJOR;
              buff[1] = LPTS_VERSION_MINOR;
              writebytes(buff, 2);
              break;
            case LPTS_QUERY_DISKS: {
              uint32_t w = get_disks();
              buff[0] = w >> 24;
              buff[1] = (w >> 16) & 0xff;
              buff[2] = (w >> 8) & 0xff;
              buff[3] = w  & 0xff;
              writebytes(buff, 4);
              break;
            }
            case LPTS_QUERY_DISK_CAPS: {
              uint32_t w;
              readbytes(buff, 1);
              w = get_disk_blocks(buff[0]);
              buff[0] = w >> 24;
              buff[1] = (w >> 16) & 0xff;
              buff[2] = (w >> 8) & 0xff;
              buff[3] = w  & 0xff;
              writebytes(buff, 4);
              break;
            }
            case LPTS_READ_BLOCK: {
              readbytes(buff, 5);
              
              uint8_t drv = buff[0];
              uint32_t lba = (buff[1] << 24) | (buff[2] << 16) | (buff[3] << 8) | buff[4];
              uint8_t r = read_disk_sector(drv, lba, buff);
              
              writebyte(r);
              if (!r) {
                writebytes(buff, 512);
              }
              break;
            }
            case LPTS_WRITE_BLOCK: {
              readbytes(buff, 5);
              
              uint8_t drv = buff[0];
              uint32_t lba = (buff[1] << 24) | (buff[2] << 16) | (buff[3] << 8) | buff[4];
              
              readbytes(buff, 512);
              uint8_t r = write_disk_sector(drv, lba, buff);
              
              writebyte(r);
              break;
            }
              
          }
          
        }
      }
      printf("Leaving protocol mode.\n");
    }
  }
  
// 	wtsynth_Kill();
  reset_usb_boot(0, 0);
	return 0;
}


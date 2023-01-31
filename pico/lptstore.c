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

int readbyte() {
  int c, d;
  while (((c = readlpt()) & 0x10) == 0x00)
    tight_loop_contents();
  d = (c & 0xf) << 4;
  gpio_put(19, 1);
  while (((c = readlpt()) & 0x10) == 0x10)
    tight_loop_contents();
  d |= (c & 0xf);
  gpio_put(19, 0);
  return d;
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

  }
  
// 	wtsynth_Kill();
  reset_usb_boot(0, 0);
	return 0;
}


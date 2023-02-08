#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <pico/time.h>

#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
// #include "hardware/flash.h"

// #include "pico/bootrom.h"
#include "pico/stdlib.h"
// #include "pico/binary_info.h"
// #include "pico/bootrom.h"

int lptcomms_init() {
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
  return 0;
}

int lptcomms_readlpt() {
  uint32_t dw = gpio_get_all() >> 20;
  uint8_t b = (dw >> 8) | ((dw&0x80) >> 6) | ((dw&0x40) >> 4) | ((dw&0x04) << 1) | ((dw&0x02) << 3);
  return b;
}

int lptcomms_hasbyte() {
  return ((lptcomms_readlpt() & 0x10) == 0x10);
}

#define PROTOCOL_TIMEOUT 200000

static int waittimeout(int mask, int value) {
  int c;
  uint64_t time_now;
  
  time_now = time_us_64();
  while (((c = lptcomms_readlpt()) & mask) == value && time_us_64() < (time_now + PROTOCOL_TIMEOUT))
    tight_loop_contents();
  
  return (c & mask) == value ? -1 : c;
}

int lptcomms_readbyte() {
  int c,d;
  
  // wait for host to signal 1 nybble ready, acknowledge 1
  c = waittimeout(0x10, 0x00);
  if (c < 0) {
    gpio_put(19, 0);
    return -1;
  }
  d = (lptcomms_readlpt() & 0xf) << 4;
  gpio_put(19, 1);

  c = waittimeout(0x10, 0x10);
  if (c < 0) {
    return -1;
  }

  d |= (lptcomms_readlpt() & 0xf);
  gpio_put(19, 0);
  
  return d;
}

// int lptcomms_readbyte() {
//   uint8_t b;
//   int i;
//   
//   i = lptcomms_readnybble();
//   if (i < 0) {
//     return -1;
//   }
//   b = i << 4;
// 
//   i = lptcomms_readnybble();
//   if (i < 0) {
//     return -1;
//   }
//   b |= i;
//   return b;
// }

void lptcomms_writelpt(uint8_t b) {
  gpio_put(16, (b & 0x01) ? 0 : 1);
  gpio_put(17, (b & 0x02) ? 0 : 1);
  gpio_put(18, (b & 0x04) ? 0 : 1);
  gpio_put(20, (b & 0x08) ? 0 : 1);
  gpio_put(19, (b & 0x10) ? 1 : 0);
}


int lptcomms_readbytes(uint8_t *buff, int len) {
  int i, r;
  for (i=0; i<len; i++) {
    r = lptcomms_readbyte();
    if (r < 0) return -1;
    buff[i] = r;
  }
  return 0;
}

int lptcomms_writebyte(uint8_t b) {
  int c,d;
  
  gpio_put(16, (b & 0x10) ? 0 : 1);
  gpio_put(17, (b & 0x20) ? 0 : 1);
  gpio_put(18, (b & 0x40) ? 0 : 1);
  gpio_put(20, (b & 0x80) ? 0 : 1);

  gpio_put(19, 1); // write upper nybble then raise signal
  c = waittimeout(0x10, 0x00);
  if (c < 0) {
    gpio_put(19, 0);
    return -1;
  }

  gpio_put(16, (b & 0x1) ? 0 : 1);
  gpio_put(17, (b & 0x2) ? 0 : 1);
  gpio_put(18, (b & 0x4) ? 0 : 1);
  gpio_put(20, (b & 0x8) ? 0 : 1);

  gpio_put(19, 0); // write upper nybble then raise signal
  c = waittimeout(0x10, 0x10);
  if (c < 0) {
    gpio_put(19, 0);
    return -1;
  }

  return 0;
}

int lptcomms_writebytes(uint8_t *d, int len) {
  int i;
  for (i=0; i<len; i++) {
    if (lptcomms_writebyte(d[i]) < 0) {
      return -1;
    }
  }
  return 0;
}

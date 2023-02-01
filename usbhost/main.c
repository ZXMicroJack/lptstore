/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#define CFG_TUH_CDC 0

// #include <stdio.h>
// #include <stdint.h>
// #include <pico/time.h>
// 
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
// #include "hardware/flash.h"

#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"


//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
void led_blinking_task(void);

extern void cdc_task(void);
extern void hid_app_task(void);


uint8_t sector[512];
uint8_t sector2[512];

extern int read_sector(int pdrv, uint8_t *buff, uint32_t sector);
extern int write_sector(int pdrv, uint8_t *buff, uint32_t sector);

/*------------- MAIN -------------*/
int main(void)
{
  stdio_init_all();
  uart_init (uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);


  board_init();

  printf("TinyUSB Host CDC MSC HID Example\r\n");

  tusb_init();

  while (1)
  {
    // tinyusb host task
    tuh_task();
    led_blinking_task();
    
    switch(getchar_timeout_us(0)) {
      case 'r': {
        printf("read: returns %d\r\n", read_sector(0, sector, 0));
        break;
      }
      case 'R': {
        printf("read2: returns %d\r\n", read_sector(0, sector2, 0));
        break;
      }
      case 'w': {
        printf("write: returns %d\r\n", write_sector(0, sector, 0));
        break;
      }
      case 'W': {
        printf("write2: returns %d\r\n", write_sector(0, sector2, 0));
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
      
//       default: {
//         printf("What?\r\n");
//       }
    }

    
    
#if CFG_TUH_CDC
    cdc_task();
#endif

#if CFG_TUH_HID
    hid_app_task();
#endif
  }

  return 0;
}

#if 0
static scsi_inquiry_resp_t inquiry_resp;

bool inquiry_complete_cb(uint8_t dev_addr, msc_cbw_t const* cbw, msc_csw_t const* csw) {
  if (csw->status != 0) {
      printf("Inquiry failed\r\n");
      return false;
  }

  // Print out Vendor ID, Product ID and Rev
  printf("%.8s %.16s rev %.4s\r\n", inquiry_resp.vendor_id, inquiry_resp.product_id, inquiry_resp.product_rev);

  // Get capacity of device
  uint32_t const block_count = tuh_msc_get_block_count(dev_addr, cbw->lun);
  uint32_t const block_size = tuh_msc_get_block_size(dev_addr, cbw->lun);

  printf("Disk Size: %lu MB\r\n", block_count / ((1024*1024)/block_size));
  printf("Block Count = %lu, Block Size: %lu\r\n", block_count, block_size);

  return true;
}

void tuh_msc_mount_cb(uint8_t dev_addr) {
  printf("A MassStorage device is mounted\n");
  uint8_t pdrv = dev_addr-1;
  uint8_t const lun = 0;
  tuh_msc_inquiry(dev_addr, lun, &inquiry_resp, inquiry_complete_cb);
  printf("drive %u mounted\r\n",pdrv);
}

void tuh_msc_umount_cb(uint8_t dev_addr) {
  uint8_t pdrv = dev_addr-1;
  printf("A MassStorage device is unmounted\r\n");
  printf("FATFS drive %u unmounted\r\n",pdrv);
}
#endif
//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
#if CFG_TUH_CDC
CFG_TUSB_MEM_SECTION static char serial_in_buffer[64] = { 0 };

void tuh_mount_cb(uint8_t dev_addr)
{
  // application set-up
  printf("A device with address %d is mounted\r\n", dev_addr);

  tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true); // schedule first transfer
}

void tuh_umount_cb(uint8_t dev_addr)
{
  // application tear-down
  printf("A device with address %d is unmounted \r\n", dev_addr);
}

// invoked ISR context
void tuh_cdc_xfer_isr(uint8_t dev_addr, xfer_result_t event, cdc_pipeid_t pipe_id, uint32_t xferred_bytes)
{
  (void) event;
  (void) pipe_id;
  (void) xferred_bytes;

  printf(serial_in_buffer);
  tu_memclr(serial_in_buffer, sizeof(serial_in_buffer));

  tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true); // waiting for next data
}

void cdc_task(void)
{

}

#endif

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  const uint32_t interval_ms = 1000;
  static uint32_t start_ms = 0;

  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
  printf("Blink!\n");
}

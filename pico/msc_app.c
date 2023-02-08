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

#include "tusb.h"
#include "fat32.h"

// #if CFG_TUH_MSC

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+
static scsi_inquiry_resp_t inquiry_resp;

static volatile int complete = 1;
static volatile int result = 0;
static int usb_disk_inserted = 0;
static int usb_disk_insert_event = 0;
static int prefetching_lba = -1;
static int prefetching_buff[512];
static volatile int prefetching_complete = 1;

bool msc_fat_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const *d)
{
    (void)dev_addr;
    complete = 1;
    prefetching_complete = 1;
    result = d->csw->status == MSC_CSW_STATUS_PASSED ? 0 : 1;
    return d->csw->status == MSC_CSW_STATUS_PASSED;
}

// External functionality for FAT32
#define disk 0
int read_sector(int sector, uint8_t *buff) {
  if (!usb_disk_inserted) return 1;

  // wait for prefetch to complete
  while (!prefetching_complete)
    tight_loop_contents();
  
  if (prefetching_lba == sector && !result) {
    memcpy(buff, prefetching_buff, 512);
  } else {
    complete = 0;

    if (!tuh_msc_read10(disk+1, 0, buff, sector, /*count*/1, msc_fat_complete_cb, (uintptr_t)NULL)) {
      return 1;
    }
    
    while (!complete)
      tight_loop_contents();
  }
  
  prefetching_lba = sector + 1;
  prefetching_complete = 0;
  if (!tuh_msc_read10(disk+1, 0, prefetching_buff, prefetching_lba, /*count*/1, msc_fat_complete_cb, (uintptr_t)NULL)) {
    return 1;
  }
  return result;
}

int write_sector(int sector, uint8_t *buff) {
  if (!usb_disk_inserted) return 1;
  complete = 0;
  if (!tuh_msc_write10(disk+1, 0, buff, sector, /*count*/1, msc_fat_complete_cb, (uintptr_t)NULL)) {
    return 1;
  }
  while (!complete)
    tight_loop_contents();
  return result;
}
#undef disk


int disk_write_sector(uint8_t disk, int lbn, uint8_t *buff) {
  return write_disk_sector(disk, lbn, buff);
}

int disk_read_sector(uint8_t disk, int lbn, uint8_t *buff) {
  return read_disk_sector(disk, lbn, buff);
}

uint32_t disk_get_blocks(uint8_t disk) {
  if (!usb_disk_inserted) return 0;
  return get_disk_blocks(disk);
}

uint32_t disk_get_drives(void) {
  if (!usb_disk_inserted) return 0;
  return get_disks();
}

int disk_init(void) {
}

/*
TinyUSB Host CDC MSC HID Example
A MassStorage device is mounted
F8 T5 rev 1.10
Disk Size: 31 MB
Block Count = 63488, Block Size: 512
*/

// bool inquiry_complete_cb(uint8_t dev_addr, msc_cbw_t const* cbw, msc_csw_t const* csw)
bool inquiry_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const* cb_data)
{
  if (cb_data->csw->status != 0)
  {
    printf("Inquiry failed\r\n");
    return false;
  }

  // Print out Vendor ID, Product ID and Rev
  printf("%.8s %.16s rev %.4s\r\n", inquiry_resp.vendor_id, inquiry_resp.product_id, inquiry_resp.product_rev);

  // Get capacity of device
  uint32_t const block_count = tuh_msc_get_block_count(dev_addr, cb_data->cbw->lun);
  uint32_t const block_size = tuh_msc_get_block_size(dev_addr, cb_data->cbw->lun);

  printf("Disk Size: %lu MB\r\n", block_count / ((1024*1024)/block_size));
  printf("Block Count = %lu, Block Size: %lu\r\n", block_count, block_size);


  usb_disk_insert_event = 1;
  return true;
}

// typedef struct {
//   msc_cbw_t const* cbw; // SCSI command
//   msc_csw_t const* csw; // SCSI status
//   void* scsi_data;      // SCSI Data
//   uintptr_t user_arg;   // user argument
// }tuh_msc_complete_data_t;
#if 0
bool msc_fat_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const *d)
{
    (void)dev_addr;
    // success = csw->status == MSC_CSW_STATUS_PASSED;
    printf("msc_fat_complete_cb: status = %d\n", d->csw->status == MSC_CSW_STATUS_PASSED);
    printf("msc_fat_complete_cb: csw->status = %d\n", d->csw->status);
    
    return d-> csw->status == MSC_CSW_STATUS_PASSED;
}
#endif

//------------- IMPLEMENTATION -------------//
void tuh_msc_mount_cb(uint8_t dev_addr)
{
  printf("A MassStorage device is mounted\r\n");
  uint8_t const lun = 0;
  tuh_msc_inquiry(dev_addr, lun, &inquiry_resp, inquiry_complete_cb, (uintptr_t)NULL);
  usb_disk_inserted = 1;
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
  (void) dev_addr;
  printf("A MassStorage device is unmounted\r\n");
  usb_disk_inserted = 0;
}

void msc_task() {
  if (usb_disk_insert_event) {
    init_disk();
    usb_disk_insert_event = 0;
  }
}

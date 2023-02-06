#include <stdio.h>
#include <stdint.h>

#include "lptcomms.h"
#include "disk.h"
#include "lptprotocol.h"

static uint8_t buff[512];

void lptprotocol_task() {
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

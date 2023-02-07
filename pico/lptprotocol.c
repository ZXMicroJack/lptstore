#include <stdio.h>
#include <stdint.h>

#include "lptcomms.h"
#include "disk.h"
#include "lptprotocol.h"

static uint8_t buff[512];
static uint8_t compress_buff[512];

int LPTCompress(uint8_t *out, uint8_t *data) {
  int c = 0, i, r = 0;
  for (i=0; i<512; i++) {
    if (data[i] == 0x00) {
      if (c == 0) {
        out[r++] = 0x00;
        c++;
      } else if (c == 255) {
        out[r++] = 0xff;
        c = 0;
      } else {
        c++;
      }
    } else {
      if (c) {
        out[r++] = c-1;
        c = 0;
      }
      out[r++] = data[i];
    }

    if (r >= 512) {
      return 0;
    }
  }
  if (c) {
    out[r++] = c-1;
  }
  return r >= 512 ? 0 : r;
}

int LPTDecompress(uint8_t *out, uint8_t *data, int len) {
  int c = 0, i, r = 0;
  for (i=0; i<len; i++) {
    if (i > 0 && data[i-1] == 0x00 && !(i > 1 && data[i-2] == 0x00) ) {
      c = data[i] + 1;
      if ((r + c) > 512) return 0;
      while (c--)
        out[r++] = 0x00;
    } else if (data[i] != 0x00) {
      out[r++] = data[i];
    }
  }
  return r;
}

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
          
          int c;
          if (!r) {
            if (c = LPTCompress(compress_buff, buff)) {
              if (lptcomms_writebyte(0x02) < 0) break;
              if (lptcomms_writebyte(c >> 8) < 0) break;
              if (lptcomms_writebyte(c & 0xff) < 0) break;
              lptcomms_writebytes(compress_buff, c);
            } else {
              if (lptcomms_writebyte(0x00) < 0) break;
              lptcomms_writebytes(buff, 512);
            }
          } else {
            lptcomms_writebyte(0x01);
          }
          break;
        }
        case LPTS_WRITE_BLOCK: {
          if (lptcomms_readbytes(buff, 5) < 0) break;
          
          uint8_t drv = buff[0];
          uint32_t lba = (buff[3] << 8) | buff[4];
          uint16_t compressed = (buff[1] << 8) | buff[2];

          uint8_t r;
          if (compressed == 0) {
            if (lptcomms_readbytes(buff, 512) < 0) break;
          } else {
            if (lptcomms_readbytes(compress_buff, compressed) < 0) break;
            LPTDecompress(buff, compress_buff, compressed);
          }
          r = disk_write_sector(drv, lba, buff);
          
          // this will timeout and break anyway.
          lptcomms_writebyte(r); 
          break;
        }
          
      } // switch
    } // if
  } // if hasbyte
}

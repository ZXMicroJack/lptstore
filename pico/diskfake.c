#include <stdio.h>
#include <string.h>
#include "disk.h"
 
int disk_init(void) {
  return 0;
}

uint32_t disk_get_drives(void) {
  return 0xffff;
}

uint32_t disk_get_blocks(uint8_t disk) {
  return 32*1024*2;
}

#include "../dos/SDPP10/block0.h"
#include "../dos/SDPP10/block4.h"

#define MAX_BLOCKS 200

static uint8_t blocks[MAX_BLOCKS][512];
static uint16_t blockNr[MAX_BLOCKS];
static uint8_t blockUnit[MAX_BLOCKS];
static int nrBlocks = 0;


int disk_read_sector(uint8_t disk, int lbn, uint8_t *buff) {
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

int disk_write_sector(uint8_t disk, int lbn, uint8_t *buff) {
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

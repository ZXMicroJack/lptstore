#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "fat32.h"

// #define debug(a) printf a

#ifndef debug
#define debug(a)
#endif

#ifdef DEBUG
void dump_sector(uint8_t *buff) {
  int i, j;
  for (i=0; i<32; i++) {
    printf("%04X : ", i*16);
    for (j=0; j<16; j++) {
      printf("%02X ", buff[i*16+j]);
    }
    printf(" : ");
    for (j=0; j<16; j++) {
      printf("%c", buff[i*16+j] < ' ' ? '.' : buff[i*16+j]);
    }
    printf("\n");
  }
}

void dump_fat32(fat32_t *pfat32) {
  printf("sectorsPerCluster %u\n", pfat32->sectorsPerCluster);
  printf("reservedSectorCount %u\n", pfat32->reservedSectorCount);
  printf("sectorsPerFAT %u\n", pfat32->sectorsPerFAT);
  printf("rootDirectoryFirstCluster %u\n", pfat32->rootDirectoryFirstCluster);

  for (int i=0; i < pfat32->nrFiles; i++) {
    printf("dirent:%s lfn:%s attr:%02X cll:%04X clh:%04X size:%08X\n",
      pfat32->file[i].name, pfat32->file[i].lfn,
      pfat32->file[i].attr, pfat32->file[i].startClusterLo,
      pfat32->file[i].startClusterHi, pfat32->file[i].fileSize);
  }
}
#endif

////////////////////////////////////////////////////////////////////////////
// main part of code
static uint32_t get_next_cluster(fat32_t *pfat32, int cluster) {
  if (pfat32->fat.offset == (uint32_t)-1 || cluster < pfat32->fat.offset || cluster > pfat32->fat.offset+127) {
    read_sector(cluster / 128 + pfat32->reservedSectorCount, (uint8_t *)pfat32->fat.rec);
    pfat32->fat.offset = cluster & 0xffffff80;

#ifdef DEBUG
    for (int i=0; i<128; i++) {
      printf("fat %08X: %08X\n", i + pfat32->fat.offset, pfat32->fat.rec[i] );
    }

    dump_sector((uint8_t *)pfat32->fat.rec);
#endif
  }
  return pfat32->fat.rec[cluster & 0x7f];
}

#define NR_DISKS  16

static uint32_t disk_bitmap = 0;

static uint32_t disk_cluster[NR_DISKS] = {0};
static uint32_t disk_lba[NR_DISKS] = {0};
static uint32_t disk_blocks[NR_DISKS] = {0};

static uint32_t cfg_lba = 0;
static uint32_t cfg_blocks = 0;
static uint32_t cfg_cluster = 0;

static int get_lba(fat32_t *pfat32, uint32_t cluster) {
  debug(("get_lba: cluster=%08X\n", cluster));
  return pfat32->reservedSectorCount + pfat32->sectorsPerFAT * 2 +
    pfat32->sectorsPerCluster * cluster - 2;
}


static void process_dir_sector(fat32_t *pfat32, uint8_t *buff) {
  for (int i=0; i<512; i+=32) {
    if (buff[i] == 0x00) break;
    if (buff[i] == 0xe5) continue;

    if (buff[i+11] != 0x0f) {
#ifdef DEBUG
      char name[12];
      memcpy(name, &buff[i], 11);
      name[11] = '\0';
      printf("name %s\n", name);
#endif
      uint16_t cluster_hi = buff[i+20] | (buff[i+21]<<8);
      uint16_t cluster_lo = buff[i+26] | (buff[i+27]<<8);
      uint32_t file_size = buff[i+28] | (buff[i+29]<<8) | (buff[i+30]<<16) | (buff[i+31]<<24);
      
      if (!memcmp(&buff[i], "DISK", 4) && !memcmp(&buff[i+5], "   DAT", 6) && (buff[i+11] & 0x20) == 0x20) {
        int n = buff[i+4] - 'A';
        disk_lba[n] = get_lba(pfat32, (cluster_hi << 16) | cluster_lo);
        disk_blocks[n] = file_size >> 9;
        disk_cluster[n] = (cluster_hi << 16) | cluster_lo;
        printf("Disk %c found lba %08X blocks %08X\n", 'A' + n, disk_lba[n], disk_blocks[n]);
      }
      
      if (!memcmp(&buff[i], "CONFIG  BIN", 11) && (buff[i+11] & 0x20) == 0x20) {
        cfg_lba = get_lba(pfat32, (cluster_hi << 16) | cluster_lo);
        cfg_blocks = file_size >> 9;
        cfg_cluster = (cluster_hi << 16) | cluster_lo;
        printf("Config found found lba %08X blocks %08X\n", cfg_lba, cfg_blocks);
      }
    }
  }
}

static int read_fs_id(fat32_t *pfat32) {
  uint8_t buff[512];
  read_sector(0, buff);
  if (buff[0x1fe] == 0x55 && buff[0x1ff] == 0xaa) {
    pfat32->sectorsPerCluster = buff[0x0d];
    pfat32->reservedSectorCount = buff[0x0e] | (buff[0x0f]<<8);
    pfat32->sectorsPerFAT = buff[0x24] | (buff[0x25]<<8) | (buff[0x26]<<16) | (buff[0x27]<<24);
    pfat32->rootDirectoryFirstCluster = buff[0x2c] | (buff[0x2d]<<8) | (buff[0x2e]<<16) | (buff[0x2f]<<24);
    return 0;
  }
  return 1;
}

static int file_has_gaps(fat32_t *pfat32, uint32_t cluster) {
  uint8_t buff[512];
  uint32_t last_lba = 0;
  while (cluster < 0x0ffffff0 && cluster > 0) {
    uint32_t lba = get_lba(pfat32, cluster);
    for (int i=0; i<pfat32->sectorsPerCluster; i++) {
      debug(("read lba %08X\n", lba+i));
      if (last_lba != 0) {
        if ((last_lba + 1) != (lba+i)) {
          // found a gap
          return 1;
        }
      }
      last_lba = lba + i;
    }
    cluster = get_next_cluster(pfat32, cluster);
  }
  return 0;
}

void read_file(fat32_t *pfat32, uint32_t cluster, void (*callback)(fat32_t *, uint8_t *)) {
  uint8_t buff[512];
  while (cluster < 0x0ffffff0 && cluster > 0) {
    uint32_t lba = get_lba(pfat32, cluster);
    for (int i=0; i<pfat32->sectorsPerCluster; i++) {
      debug(("read lba %08X\n", lba+i));
      read_sector(lba+i, buff);
      callback(pfat32, buff);
    }
    cluster = get_next_cluster(pfat32, cluster);
  }
}


////////////////////////////////////////////////////////////////////////////
// external API

uint32_t get_disks(void) {
  return disk_bitmap;
}

uint32_t get_disk_blocks(uint8_t disk) {
  return disk == CFG_DISK ? cfg_blocks :
        (disk <= NR_DISKS && disk_lba[disk]) ? disk_blocks[disk] : 0;
}

int read_disk_sector(uint8_t disk, int sector, uint8_t *buff) {
  int result = -1;
  
  if (disk == CFG_DISK && cfg_lba > 0 && sector < cfg_blocks) {
    result = read_sector(cfg_lba + sector, buff);
  } else if (disk <= NR_DISKS && disk_lba[disk] > 0 && sector < disk_blocks[disk]) {
    result = read_sector(disk_lba[disk] + sector, buff);
  } else {
    // out of range or inactive or invalid disk
  }
  return result;
}

int write_disk_sector(uint8_t disk, int sector, uint8_t *buff) {
  int result = -1;
  
  if (disk == CFG_DISK && cfg_lba > 0 && sector < cfg_blocks) {
    result = write_sector(cfg_lba + sector, buff);
  } else if (disk <= NR_DISKS && disk_lba[disk] > 0 && sector < disk_blocks[disk]) {
    result = write_sector(disk_lba[disk] + sector, buff);
  } else {
    // out of range or inactive or invalid disk
  }
  return result;
}


int init_disk(void) {
  fat32_t fat32;
  int i;
  int nr_good = 0;
  int cfg_good = 0;
  
  disk_bitmap = 0;
  
  if (read_fs_id(&fat32)) {
    debug(("FS not good\n"));
    return 1;
  }
  
#ifdef DEBUG
  dump_fat32(&fat32);
#endif
  
  read_file(&fat32, fat32.rootDirectoryFirstCluster, process_dir_sector);
  
  /* now check each disk to make sure it has contigious lbas */
  for (i=0; i<NR_DISKS; i++) {
    if (disk_lba[i]) {
      printf("Check disk %c for contigious sectors ... ", 'A' + i);
      if (file_has_gaps(&fat32, disk_cluster[i])) {
        printf("FAILED\nNon-contigious disk file too much bother.\n");
        disk_lba[i] = 0;
        disk_cluster[i] = 0;
      } else {
        printf("GOOD :-)\n");
        nr_good ++;
        disk_bitmap |= 1 << i;
      }
    }
  }

  if (cfg_cluster) {
    printf("Check config for contigious sectors ... ");
    if (file_has_gaps(&fat32, cfg_cluster)) {
      printf("FAILED\nNon-contigious config file too much bother.\n");
      cfg_cluster = 0;
    } else {
      printf("GOOD :-)\n");
      cfg_good = 1;
      disk_bitmap |= CONFIG_BLOCK;
    }
  }
  
  if (nr_good == 0) {
    printf("ERROR: There are no good disks found on this drive\n"
          "please make files in the root directory DISKn.DAT\n");
  } else {
    printf("INFO: %d / %d possible disks active\n", nr_good, NR_DISKS);
  }

  if (cfg_good == 0) {
    printf("ERROR: There was no config block found on this drive\n"
          "please make a file in the root directory CONFIG.DAT\n");
  } else {
    printf("INFO: Config is good.\n");
  }
  
  return 0;
}

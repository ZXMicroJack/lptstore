#ifndef _FAT32_H
#define _FAT32_H

typedef struct {
  uint32_t offset;
  uint32_t rec[128];
} fat_t;

typedef struct {
    uint8_t sectorsPerCluster;
    uint16_t reservedSectorCount;
    uint32_t sectorsPerFAT;
    uint32_t rootDirectoryFirstCluster;
    fat_t fat;
} fat32_t;

// External functionality
int read_sector(int sector, uint8_t *buff);
int write_sector(int sector, uint8_t *buff);

// debug functionality
void dump_sector(uint8_t *buff);
void dump_fat32(fat32_t *pfat32);

#define CONFIG_BLOCK  0x80000000
#define NR_DISKS  16
#define CFG_DISK  0xff

int init_disk(void);
uint32_t get_disks(void);
uint32_t get_disk_blocks(uint8_t disk);
int read_disk_sector(uint8_t disk, int sector, uint8_t *buff);
int write_disk_sector(uint8_t disk, int sector, uint8_t *buff);

#endif

#ifndef _DISK_H
#define _DISK_H

int disk_write_sector(uint8_t disk, int lbn, uint8_t *buff);
int disk_read_sector(uint8_t disk, int lbn, uint8_t *buff);
uint32_t disk_get_blocks(uint8_t disk);
uint32_t disk_get_drives(void);
int disk_init(void);

#endif



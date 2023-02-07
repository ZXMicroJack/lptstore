#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "fat32.h"

#define passif(a) { int r = (a); if (r) { printf("[%d]%s " #a "\n", __LINE__, r ? "PASSED" : "FAILED" ); } }

// #define CARDFILE    "card"

#define CARDFILE    "/dev/sdb"

int read_sector(int sector, uint8_t *buff) {
  FILE *f = fopen(CARDFILE, "rb");
  fseek(f, sector*512, SEEK_SET);
  fread(buff, 1, 512, f);
  fclose(f);
  return 0;
}

int write_sector(int sector, uint8_t *buff) {
#if 0
  FILE *f = fopen(CARDFILE, "wb+");
  fseek(f, sector*512, SEEK_SET);
  fwrite(buff, 1, 512, f);
  fclose(f);
#endif
  return 0;
}



int main(void) {
  passif(init_disk() == 0);
  return 0;
}

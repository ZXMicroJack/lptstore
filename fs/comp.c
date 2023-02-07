#include <stdio.h>
#include <stdint.h>
#include <string.h>


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

int main(int argc, char **argv) {
	FILE *f = fopen(argv[1], "rb");
	FILE *f2 = fopen("output.bin", "wb");
	uint8_t data[512];
	uint8_t cdata[512];
  
  int firstone = 1;

	while (!feof(f)) {
		memset(data, 0, sizeof data);
		memset(cdata, 0, sizeof data);
		int a = fread(data, 1, sizeof data, f);
		int c = LPTCompress(cdata, data);
    
    if (firstone) {
      printf("Uncompressed\n--------------------------------\n");
      dump_sector(data);
      printf("Compressed\n--------------------------------\n");
      printf("c = %d\n", c);
      dump_sector(cdata);
    }
    
		if (!c) {
			fwrite(data, 1, a, f2);
		} else {
			memset(data, 0, sizeof data);
			LPTDecompress(data, cdata, c);
      if (firstone) {
        printf("Decompressed\n--------------------------------\n");
        dump_sector(data);
      }
			fwrite(data, 1, a, f2);
		}
		firstone = 0;
	}

	fclose(f);
	fclose(f2);
	return 0;
}

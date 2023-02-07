/* sd.h - SD card driver glue */

#ifndef _SD_H
#define _SD_H

#include "integer.h"
#include "driver.h"

#define BLOCKSIZE 512

/* Status of Disk Functions */
typedef BYTE   DSTATUS;

/* Results of Disk Functions */
typedef enum {
   RES_OK = 0,    /* 0: Successful */
   RES_ERROR,     /* 1: R/W Error */
   RES_WRPRT,     /* 2: Write Protected */
   RES_NOTRDY,    /* 3: Not Ready */
   RES_PARERR     /* 4: Invalid Parameter */
} DRESULT;


extern DWORD partition_offset;

/* SDInitialize - establish two way communications with the drive */
BOOLEAN LPTInitialize (BYTE unit, BYTE partno, bpb_t *bpb);
/* SDRead - read one 512 byte logical block from the tape */
int LPTRead (WORD, WORD, BYTE far *, WORD count);
/* SDWrite - write one 512 byte logical block to the tape */
int LPTWrite (WORD, WORD, BYTE far *, WORD count);
/* SDMediaCheck - check if media changed */
BOOLEAN LPTMediaCheck (BYTE unit);

PUBLIC void LPTInit(void);

#endif

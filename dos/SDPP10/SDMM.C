/*------------------------------------------------------------------------/
/  Foolproof MMCv3/SDv1/SDv2 (in SPI mode) control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2013, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------/
  Features and Limitations:

  * Easy to Port Bit-banging SPI
    It uses only four GPIO pins. No interrupt, no SPI is needed.

  * Platform Independent
    You need to modify only a few macros to control GPIO ports.

  * Low Speed
    The data transfer rate will be several times slower than hardware SPI.

  * No Media Change Detection
    Application program needs to perform f_mount() after media change.

/-------------------------------------------------------------------------*/

#include <conio.h>

#include "diskio.h"     /* Common include file for FatFs and disk I/O layer */
#include "cprint.h"

/*-------------------------------------------------------------------------*/
/* Platform dependent macros and functions needed to be modified           */
/*-------------------------------------------------------------------------*/

#define outportbyte(a,b) outp(a,b)
#define inportbyte(a) inp(a)

static WORD portbases[5] = {0x3BC,0x378,0x278,0x3E8,0x2E8};

BYTE sd_card_check = 0;
BYTE portbase = 2;

WORD OUTPORT=0x378;
WORD STATUSPORT=0x379;
WORD CONTROLPORT=0x37A;


static
DSTATUS Stat = STA_NOINIT; /* Disk status */

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

// #include "block0.h"
// #include "block4.h"

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
   BYTE drv       /* Drive number (always 0) */
)
{
//    return STA_NOINIT;
   return 0;
}

DRESULT disk_result (
   BYTE drv       /* Drive number (always 0) */
)
{
   return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
   BYTE drv    /* Physical drive nmuber (0) */
)
{
  return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
   BYTE drv,            /* Physical drive nmuber (0) */
   BYTE DOSFAR *buff,   /* Pointer to the data buffer to store read data */
   DWORD sector,        /* Start sector number (LBA) */
   UINT count           /* Sector count (1..128) */
)
{
//   if (sector == 0) fmemcpy(buff, xaa, 512);
//   else if (sector == 4) fmemcpy(buff, xae, 512);
//   else fmemset(buff, 0, 512);
  
  return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
   BYTE drv,                /* Physical drive nmuber (0) */
   const BYTE DOSFAR *buff, /* Pointer to the data to be written */
   DWORD sector,            /* Start sector number (LBA) */
   UINT count               /* Sector count (1..128) */
)
{
  return RES_OK;
}

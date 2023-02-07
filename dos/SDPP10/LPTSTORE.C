/* Copyright (C) 1994 by Robert Armstrong          */
/*                         */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or */
/* (at your option) any later version.             */
/*                         */
/* This program is distributed in the hope that it will be useful, but  */
/* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT- */
/* ABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General */
/* Public License for more details.             */
/*                         */
/* You should have received a copy of the GNU General Public License */
/* along with this program; if not, visit the website of the Free */
/* Software Foundation, Inc., www.gnu.org.            */
/*                         */
/*                         */
/*   The functions provided are:             */
/*                         */
/* LPTInitialize - establish two way communications with the drive */
/* LPTRead       - read one 512 byte logical block from the tape   */
/* LPTWrite      - write one 512 byte logical block to the tape */
/* LPTMediaCheck - see if card detect has changed */
/*                         */
/*   Normally the SDInitialize routine would be called  */
/* during the DOS device driver initialization, and then the SDRead and */
/*                         */

#include <stdio.h>      /* needed for NULL, etc       */
#include <mem.h>        /* memset, memcopy, etc       */
#include <dos.h>

#include "standard.h"   /* all definitions for this project */
#include "lptstore.h"         /* device protocol and data defintions */
// #include "diskio.h"     /* stuff from sdmm.c module */
#include "driver.h"
#include "cprint.h"

static WORD portbases[5] = {0x3BC,0x378,0x278,0x3E8,0x2E8};

BYTE sd_card_check = 0;
BYTE portbase = 1;
DWORD partition_offset = 0;

int LPTBASE = 0x378;
// #define FAKE_LPT
extern BOOLEAN Debug;
/* FatFs refers the members in the FAT structures as byte array instead of
/ structure member because the structure is not binary compatible between
/ different platforms */

#define BS_jmpBoot         0  /* Jump instruction (3) */
#define BS_OEMName         3  /* OEM name (8) */
#define BPB_BytsPerSec     11 /* Sector size [byte] (2) */
#define BPB_SecPerClus     13 /* Cluster size [sector] (1) */
#define BPB_RsvdSecCnt     14 /* Size of reserved area [sector] (2) */
#define BPB_NumFATs        16 /* Number of FAT copies (1) */
#define BPB_RootEntCnt     17 /* Number of root directory entries for FAT12/16 (2) */
#define BPB_TotSec16    19 /* Volume size [sector] (2) */
#define BPB_Media       21 /* Media descriptor (1) */
#define BPB_FATSz16        22 /* FAT size [sector] (2) */
#define BPB_SecPerTrk      24 /* Track size [sector] (2) */
#define BPB_NumHeads    26 /* Number of heads (2) */
#define BPB_HiddSec        28 /* Number of special hidden sectors (4) */
#define BPB_TotSec32    32 /* Volume size [sector] (4) */
#define BS_DrvNum       36 /* Physical drive number (2) */
#define BS_BootSig         38 /* Extended boot signature (1) */
#define BS_VolID        39 /* Volume serial number (4) */
#define BS_VolLab       43 /* Volume label (8) */
#define BS_FilSysType      54 /* File system type (1) */
#define BPB_FATSz32        36 /* FAT size [sector] (4) */
#define BPB_ExtFlags    40 /* Extended flags (2) */
#define BPB_FSVer       42 /* File system version (2) */
#define BPB_RootClus    44 /* Root directory first cluster (4) */
#define BPB_FSInfo         48 /* Offset of FSINFO sector (2) */
#define BPB_BkBootSec      50 /* Offset of backup boot sector (2) */
#define BS_DrvNum32        64 /* Physical drive number (2) */
#define BS_BootSig32    66 /* Extended boot signature (1) */
#define BS_VolID32         67 /* Volume serial number (4) */
#define BS_VolLab32        71 /* Volume label (8) */
#define BS_FilSysType32    82 /* File system type (1) */
#define  FSI_LeadSig       0  /* FSI: Leading signature (4) */
#define  FSI_StrucSig      484   /* FSI: Structure signature (4) */
#define  FSI_Free_Count    488   /* FSI: Number of free clusters (4) */
#define  FSI_Nxt_Free      492   /* FSI: Last allocated cluster (4) */
#define MBR_Table       446   /* MBR: Partition table offset (2) */
#define  SZ_PTE            16 /* MBR: Size of a partition table entry */
#define BS_55AA            510   /* Boot sector signature (2) */


BYTE local_buffer[BLOCKSIZE];

#define LD_WORD(x) *((WORD *)(BYTE *)(x))
#define LD_DWORD(x) *((DWORD *)(BYTE *)(x))

void local_memcpy(BYTE far *dst, BYTE far *src, int n) {
  while (n--) {
    *dst++ = *src++;
  }
}

void local_memset(BYTE far *dst, BYTE d, int n) {
  while (n--) {
    *dst++ = d;
  }
}

#define fmemcpy local_memcpy
#define fmemset local_memset
/*-----------------------------------------------------------------------*/
/* Load a sector and check if it is an FAT boot sector                   */
/*-----------------------------------------------------------------------*/

static
BYTE check_fs (BYTE unit,  
   /* 0:FAT boor sector, 1:Valid boor sector but not FAT, */
   /* 2:Not a boot sector, 3:Disk error */
   DWORD sect  /* Sector# (lba) to check if it is an FAT boot record or not */
)
{
  if (LPTRead(unit, sect, local_buffer, 1) != RES_OK)
    return 3;
   if (LD_WORD(&local_buffer[BS_55AA]) != 0xAA55) /* Check boot record signature (always placed at offset 510 even if the sector size is >512) */
      return 2;
   if ((LD_DWORD(&local_buffer[BS_FilSysType]) & 0xFFFFFF) == 0x544146)      /* Check "FAT" string */
      return 0;
   if ((LD_DWORD(&local_buffer[BS_FilSysType32]) & 0xFFFFFF) == 0x544146) /* Check "FAT" string */
      return 0;
   return 1;
}


/*-----------------------------------------------------------------------*/
/* Find logical drive and check if the volume is mounted                 */
/*-----------------------------------------------------------------------*/

WORD disk_initialized = 0;

static
int find_volume ( 
   BYTE unit,
   BYTE partno,
   bpb_t *bpb
)
{
   BYTE fmt;
   WORD secsize;
   DWORD bsect;

  if (disk_initialized & (1<<unit)) {
    return 0;
  }

   /* The file system object is not valid. */
   /* Following code attempts to mount the volume. (analyze BPB and initialize the fs object) */

   /* Find an FAT partition on the drive. Supports only generic partitioning, FDISK and SFD. */
   bsect = 0;
//    cdprintf("[%d]\n", __LINE__);
   fmt = check_fs(unit, bsect);             /* Load sector 0 and check if it is an FAT boot sector as SFD */
   if (fmt == 1 || (!fmt && (partno))) { /* Not an FAT boot sector or forced partition number */
      UINT i;         
      DWORD br[4];

      for (i = 0; i < 4; i++) {        /* Get partition offset */
         BYTE *pt = &local_buffer[MBR_Table + i * SZ_PTE];
         br[i] = pt[4] ? LD_DWORD(&pt[8]) : 0;
      }
      i = partno;                /* Partition number: 0:auto, 1-4:forced */
      if (i) i--;
      do {                       /* Find an FAT volume */
         bsect = br[i];
         fmt = bsect ? check_fs(unit, bsect) : 2; /* Check the partition */
      } while (!partno && fmt && ++i < 4);
   }
   if (fmt == 3) return -2;      /* An error occured in the disk I/O layer */
   if (fmt) return -3;           /* No FAT volume is found */

   /* An FAT volume is found. Following code initializes the file system object */

   secsize = LD_WORD(local_buffer + BPB_BytsPerSec);
   if (secsize != BLOCKSIZE)
      return -3;

   bpb->sector_size = BLOCKSIZE;
   bpb->allocation_unit = local_buffer[BPB_SecPerClus];     /* Number of sectors per cluster */
   bpb->reserved_sectors = LD_WORD(local_buffer+BPB_RsvdSecCnt);  /* Number of reserved sectors */
   if (!bpb->reserved_sectors) return -3;                   /* (Must not be 0) */
   bpb->fat_count = local_buffer[BPB_NumFATs];              /* Number of FAT copies */
   if (bpb->fat_count == 0)
      bpb->fat_count = 2;
   bpb->directory_size = LD_WORD(local_buffer+BPB_RootEntCnt);   
   bpb->total_sectors = LD_WORD(local_buffer+BPB_TotSec16);
   if (!bpb->total_sectors) 
      bpb->sector_count = LD_DWORD(local_buffer+BPB_TotSec32);
   else
      bpb->sector_count = bpb->total_sectors;
   bpb->media_descriptor = local_buffer[BPB_Media];
   bpb->fat_sectors = LD_WORD(local_buffer+BPB_FATSz16);             /* Number of sectors per FAT */
   bpb->track_size = LD_WORD(local_buffer+BPB_SecPerTrk);     /* Number of sectors per cluster */
   bpb->head_count = LD_WORD(local_buffer+BPB_NumHeads);     /* Number of sectors per cluster */
   bpb->hidden_sectors = 1;

   partition_offset = bsect;
   
   cdprintf("bpb->total_sectors %d\n", bpb->total_sectors);
   cdprintf("bpb->sector_count %d\n", bpb->sector_count);
   disk_initialized |= 1<<unit;

  if (Debug)
  {   
      cdprintf("lptstore: BPB data:\n");
      cdprintf("Sector Size: %d   ", bpb->sector_size);
      cdprintf("Allocation unit: %d\n", bpb->allocation_unit);
      cdprintf("Reserved sectors: %d  ", bpb->reserved_sectors);
      cdprintf("Fat Count: %d\n", bpb->fat_count);
      cdprintf("Directory size: %d  ", bpb->directory_size);
      cdprintf("Total sectors: %d\n", bpb->total_sectors);
      cdprintf("Media descriptor: %x  ", bpb->media_descriptor);
      cdprintf("Fat sectors: %d\n", bpb->fat_sectors);
      cdprintf("Track size: %d  ", bpb->track_size);
      cdprintf("Head count: %d\n", bpb->head_count);
      cdprintf("Hidden sectors: %d  ", bpb->hidden_sectors);
      cdprintf("Sector Ct 32 hex: %L\n", bpb->sector_count);
      cdprintf("Partition offset: %L\n", partition_offset);
   }

   return 0;
}

PUBLIC BOOLEAN LPTInitialize (BYTE unit, BYTE partno, bpb_t *bpb)
{
  if (find_volume(unit,partno,bpb) < 0)
      return FALSE;
  return TRUE;
}

PUBLIC BOOLEAN LPTMediaCheck (BYTE unit)
{
  return FALSE;
}

/* SDRead */
/*  IMPORTANT!  Blocks are always 512 bytes!  Never more, never less.   */
/*                         */
/* INPUTS:                       */
/* unit  - selects tape drive 0 (left) or 1 (right)      */
/* lbn   - logical block number to be read [0..511]      */
/* buffer   - address of 512 bytes to receive the data read    */
/*                         */
/* RETURNS: operation status as reported by the TU58     */
/*                         */
#ifdef FAKE_LPT
#include "block0.h"
#include "block4.h"

#define MAX_BLOCKS 50

BYTE blocks[MAX_BLOCKS][512];
WORD blockNr[MAX_BLOCKS];
BYTE blockUnit[MAX_BLOCKS];
int nrBlocks = 0;

PUBLIC int LPTRead (WORD unit, WORD lbn, BYTE far *buffer, WORD count)
{
  int i, j;
  for (i=0; i<count; i++) {
    for (j=0; j<nrBlocks; j++) {
      if (blockUnit[j] == unit && blockNr[j] == lbn) {
        fmemcpy(buffer, blocks[j], 512);
        break;
      }
    }
    if (j>=nrBlocks) {
      if (lbn == 0) fmemcpy(buffer, xaa, 512);
      else if (lbn == 4) fmemcpy(buffer, xae, 512);
      else if (lbn == 68) fmemcpy(buffer, xae, 512);
      else fmemset(buffer, 0, 512);
    }
    lbn ++;
    buffer += 512;
  }
  return RES_OK;
}
#else
#define LPTS_HELLO            0x00
#define LPTS_QUERY_DISKS      0x01
#define LPTS_QUERY_DISK_CAPS  0x02
#define LPTS_READ_BLOCK       0x03
#define LPTS_WRITE_BLOCK      0x04
        
#define LPTS_VERSION_MAJOR    0
#define LPTS_VERSION_MINOR    1

#undef outp
#define outp outportb
#undef inp
#define inp inportb

static void init_lpt() {
  outp(LPTBASE,0x00);
}

#if 0
void write_nybble(BYTE x) {
  /* set upper nybble and raise signal bit to 1 wait for 1 */
  outp(LPTBASE, 0x10 | (x & 0xf));
  while ((inp(LPTBASE+1) & 0x80) == 0);
  outp(LPTBASE, inp(LPTBASE) & 0xf);
  while ((inp(LPTBASE+1) & 0x80) != 0);
}
#endif

BYTE read_nybble() {
  BYTE r;
  /* set upper nybble and raise signal bit to 1 wait for 1 */
  while ((inp(LPTBASE+1) & 0x80) == 0);
  r = (inp(LPTBASE+1) >> 3) & 0xf;
  outp(LPTBASE, inp(LPTBASE) | 0x10);

  while ((inp(LPTBASE+1) & 0x80) != 0);
  outp(LPTBASE, inp(LPTBASE) & 0x0f);
  return r;
}

BYTE read_byte() {
  BYTE r;
  
  _DX = LPTBASE;
  
  asm {
    INC DX;
  }
  loop1:
  asm {
    IN AL,DX;
    TEST AL,0x80;
    JZ loop1;
    IN AL,DX;
    SHL AL,1;
    AND AL,0xf0;
    MOV AH,AL;
    
    DEC DX;
    MOV AL,0x10;
    OUT DX,AL;
    INC DX;
  };
  loop2:
  asm {
    IN AL,DX;
    TEST AL,0x80;
    JNZ loop2;
    DEC DX;
    MOV AL,0x00;
    OUT DX,AL;
  }
  asm {
    INC DX;
  }
  loop3:
  asm {
    IN AL,DX;
    TEST AL,0x80;
    JZ loop3;
    IN AL,DX;
    SHR AL,1;
    SHR AL,1;
    SHR AL,1;
    AND AL,0x0f;
    OR AH,AL;
    
    DEC DX;
    MOV AL,0x10;
    OUT DX,AL;
    INC DX;
  };
  loop4:
  asm {
    IN AL,DX;
    TEST AL,0x80;
    JNZ loop4;
    DEC DX;
    MOV AL,0x00;
    OUT DX,AL;
  }
  return _AH;
#if 0
  /* set upper nybble and raise signal bit to 1 wait for 1 */
  while ((inp(LPTBASE+1) & 0x80) == 0);
  r = (inp(LPTBASE+1) >> 3) & 0xf;
  outp(LPTBASE, inp(LPTBASE) | 0x10);

  while ((inp(LPTBASE+1) & 0x80) != 0);
  outp(LPTBASE, inp(LPTBASE) & 0x0f);
  return r;
#endif
}

#if 0
BYTE read_byte() {
  BYTE r;
  r = read_nybble() << 4;
  r |= read_nybble();
  return r;
}
#endif

static void write_byte(BYTE b) {
  _DX = LPTBASE;
  _AL = b;
  asm {
    PUSH AX;
    SHR AL,1;
    SHR AL,1;
    SHR AL,1;
    SHR AL,1;
    OR AL,0x10;
    OUT DX, AL;
    INC DX;
  }
  loop1:
  asm {
    IN AL,DX;
    TEST AL,0x80;
    JZ loop1;
    
    AND AL,0x0f;
    DEC DX;
    OUT DX, AL;

    INC DX;
  }
  loop2:
  asm {
    IN AL,DX;
    TEST AL,0x80;
    JNZ loop2;
  }
  asm {
    POP AX;
    AND AL,0x0f;
    OR AL,0x10;
    DEC DX;
    OUT DX, AL;
    INC DX;
  }
  loop3:
  asm {
    IN AL,DX;
    TEST AL,0x80;
    JZ loop3;
    
    AND AL,0x0f;
    DEC DX;
    OUT DX, AL;

    INC DX;
  }
  loop4:
  asm {
    IN AL,DX;
    TEST AL,0x80;
    JNZ loop4;
  }
//   write_nybble(b >> 4);
//   write_nybble(b & 0xf);
}

PUBLIC int LPTRead (WORD unit, WORD lbn, BYTE far *buffer, WORD count)
{
  BYTE result;
  int i, j;
  int ret = RES_OK; /* optimist */
  
  for (j=0; j<count; j++) {
    write_byte(LPTS_READ_BLOCK);
    write_byte(unit);
    write_byte(0x00);
    write_byte(0x00);
    write_byte(lbn >> 8);
    write_byte(lbn & 0xff);
    result = read_byte();
    if (!result) {
      for (i=0; i<512; i++) {
        *buffer++ = read_byte();
      }
    } else {
      ret = RES_ERROR;
      break;
    }
    lbn++;
  }
  
  return ret;
}
#endif

/* SDWrite */
/*  IMPORTANT!  Blocks are always 512 bytes!  Never more, never less.   */
/*                         */
/* INPUTS:                       */
/* unit  - selects tape drive 0 (left) or 1 (right)      */
/* lbn   - logical block number to be read [0..511]      */
/* buffer   - address of 512 bytes containing the data to write   */
/* verify   - TRUE to ask the TU58 for a verification pass     */
/*                         */
/* RETURNS: operation status as reported by the TU58     */
/*                         */
#ifdef FAKE_LPT
PUBLIC int LPTWrite (WORD unit, WORD lbn, BYTE far *buffer, WORD count)
{
  int i, j;
  
  for (i=0; i<count; i++) {
    int j;
    for (j=0; j<nrBlocks; j++) {
      if (blockUnit[j] == unit && blockNr[j] == lbn) {
        fmemcpy(blocks[j], buffer, 512);
        break;
      }
    }
    if (j>=nrBlocks && nrBlocks < MAX_BLOCKS) {
      fmemcpy(blocks[nrBlocks], buffer, 512);
      blockUnit[nrBlocks] = unit;
      blockNr[nrBlocks++] = lbn;
    }
    lbn ++;
    buffer += 512;
  }
  return RES_OK;
}
#else
PUBLIC int LPTWrite (WORD unit, WORD lbn, BYTE far *buffer, WORD count)
{
  BYTE result;
  int i, j;
  int ret = RES_OK; /* optimist */
  
  for (j=0; j<count; j++) {
    write_byte(LPTS_WRITE_BLOCK);
    write_byte(unit);
    write_byte(0x00);
    write_byte(0x00);
    write_byte(lbn >> 8);
    write_byte(lbn & 0xff);
    for (i=0; i<512; i++) {
      write_byte(*buffer++);
    }
    result = read_byte();
    if (result) {
      ret = RES_ERROR;
      break;
    }
    lbn++;
  }
  
  return ret;
}
#endif

#ifdef FAKE_LPT
PUBLIC void LPTInit(void) {
}
#else
#include <bios.h>

static unsigned long timebase = 0;
static unsigned long lastticks = 0;
unsigned long get_ticks() {
  unsigned long ticks;
  _AH = 0x00;
  asm INT 0x1a;
  
  ticks = timebase + ((_CX << 16) | _DX);
  if (ticks < lastticks) {
    timebase += 0x1800B0;
  }
  lastticks = ticks;

  return timebase + ticks;
}

PUBLIC void LPTInit(void) {
  extern volatile unsigned long timer_ticks;
  unsigned long time_now;
  
  LPTBASE = portbases[portbase];
  

  cdprintf("Info: initializing parallel port\n");
  outp(LPTBASE, 0x00);
  time_now = get_ticks();
  while (get_ticks() < (time_now + 36));
  cdprintf("Info: done\n");
}
#endif

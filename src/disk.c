/* asioemu - Atari 8bit SIO disk emulator
   disk.c  - Atari disk image operations

   Copyright (C) 2013 Free Software Foundation, Inc.

   Written by Thomas Bruss, on 2013-10-27.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
   Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  
*/

/* System include files */
#include <stdio.h>
#include <string.h>

/* Program include files */
#include "main.h"
#include "disk.h"

/* Local defines */
#define LOC_MAXSECTOR_SIZE 0x100 /* Sector size maximum */
#define LOC_MAXSECTORNUM   0x410 /* Number of maximum sectors */
#define LOC_MAXIMAGE_SIZE (LOC_MAXSECTOR_SIZE * LOC_MAXSECTORNUM) /* ATR image data size */

#define LOC_HEADER_SIZE 16 /* ATR image header size */
#define LOC_OFFS_MAGIC  0  /* Magic number offset in ATR image */
#define LOC_OFFS_ISIZE  2  /* Image size offset in ATR image */
#define LOC_OFFS_SSIZE  4  /* Sector offset in ATR image */

/* Local variables */
static unsigned char locHeader[LOC_HEADER_SIZE]; /* ATR header data */
static unsigned char locImage[LOC_MAXIMAGE_SIZE]; /* ATR image data */

static int           locDiskImageSize;      /* Size of image */
static int           locDiskSectorSize;     /* Size of a sector */
static unsigned char locDiskTrackNum;       /* Number of tracks */
static int           locDiskSecPerTrack;    /* Sectors per track */
static unsigned char locDiskSideNum;        /* Number of sides */
static unsigned char locDiskMode;           /* MFM (0x4) or 0x0 */


/* Parse and verify ATR header information */
static int locCheckHeader(void) {
    int x;

    /* Magic key check */
    x = locHeader[LOC_OFFS_MAGIC] |(locHeader[LOC_OFFS_MAGIC + 1] << 8);
    if (x != 0x296) {
        printf("Magic number wrong %X\n",x);
        return -1;
    }
    locDiskImageSize = locHeader[LOC_OFFS_ISIZE] | ((int)locHeader[LOC_OFFS_ISIZE + 1] << 8);
    locDiskImageSize *= 0x10;
    if (locDiskImageSize > LOC_MAXIMAGE_SIZE) {
        printf("Image size wrong %X\n",x);
        return -1;
    }
    locDiskSectorSize = locHeader[LOC_OFFS_SSIZE] | ((int)locHeader[LOC_OFFS_SSIZE + 1] << 8);
    if ((locDiskSectorSize != 0x80) && (locDiskSectorSize != 0x100)) {
        return -1;
    }

    locDiskTrackNum    = 0x28;
    locDiskSecPerTrack = 0x12;
    locDiskSideNum     = 0x01;
    if (locDiskSectorSize == 0x80) {
        locDiskMode        = 0x00;
        x = locDiskSectorSize * locDiskTrackNum 
            * locDiskSecPerTrack * locDiskSideNum;
        if (locDiskImageSize > x) {
            locDiskSecPerTrack = 0x1A;
            locDiskMode        = 0x04;
        }
    } else {
        locDiskMode        = 0x04;
        locDiskSecPerTrack = 0x12;
    }

    return locDiskImageSize;
}

/* Update information in ATR header */
static int locUpdateHeader(void) {
    int x;

    locDiskImageSize = locDiskSectorSize * locDiskTrackNum 
        * locDiskSecPerTrack * locDiskSideNum;
    if (locDiskSectorSize != 0x80) {
        locDiskImageSize -= 3 * 0x80; /* 1st three secs have 0x80 */
    }
    x = locDiskImageSize / 0x10;
    locHeader[LOC_OFFS_ISIZE]     = x & 0xff;
    locHeader[LOC_OFFS_ISIZE + 1] = x / 256;
    locHeader[LOC_OFFS_SSIZE]     = locDiskSectorSize & 0xff;
    locHeader[LOC_OFFS_SSIZE + 1] = locDiskSectorSize / 256;
    return 0;
}

/* Translate sector number to image memory offset */
static int locSectorToImage(int Num, int *Size) {
    int offset;

    if (Num <= 0) {
        return -1;
    }
    Num--; /* $$$$: Is there a sector 0? */
    if (Num > 2) {
        *Size = locDiskSectorSize;
    } else {
        *Size = 0x80;
    }
    offset = 0;
    while (Num > 3) {
        offset += locDiskSectorSize;
        Num--;
    }
    while (Num > 0) {
        offset += 0x80;
        Num--;
    }
    return offset;
}

/* Create a disk image */
int Disk_Create(char Format, char *Name) {
    printf("Create disk image %s\n", Name);
    locHeader[LOC_OFFS_MAGIC]     = 0x96;
    locHeader[LOC_OFFS_MAGIC + 1] = 0x02;
    switch (Format) {
    case 'm':
        /* medium density */
        locDiskSecPerTrack = 0x1a;
        locDiskMode        = 0x04;
        locDiskSectorSize  = 0x80;
        break;
    case 'd':
        /* double density */
        locDiskSecPerTrack = 0x12;
        locDiskMode        = 0x04;
        locDiskSectorSize  = 0x100;
        break;
    default:
        /* single density */
        locDiskSecPerTrack = 0x12;
        locDiskMode        = 0x00;
        locDiskSectorSize  = 0x80;
        break;
    }
    locDiskTrackNum    = 0x28;
    locDiskSideNum     = 1;
    locUpdateHeader();
    memset(locImage, 0, locDiskImageSize);
    return 0;
}

/* Load an ATR image */
int Disk_Load(char *Name) {
    FILE *file;
    int   len;
    int   size;

    printf("\nLoading to image %s\n", Name);
    file = fopen(Name,"r");
    if (!file) {
        return -1;
    }
    len = fread(locHeader, 1, sizeof(locHeader), file);
    if (len > 0) {
        HexDump(locHeader, len);
    }
    size = locCheckHeader();
    if (size < 0) {
        fclose(file);
        return -1;
    }
    len = fread(locImage, 1, size, file);
    if (len <= 0) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

/* Save an ATR image */
int Disk_Save(char *Name) {
    FILE *file;
    int   len;
    int   size;

    printf("\nWriting to image %s\n", Name);
    file = fopen(Name,"wb");
    if (!file) {
        return -1;
    }
    len = fwrite(locHeader, 1, sizeof(locHeader), file);
    size = locCheckHeader();
    if (size < 0) {
        fclose(file);
        return -1;
    }
    len = fwrite(locImage, 1, size, file);
    if (len <= 0) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}


/* Format image using current configuration */
int Disk_FormatAuto(unsigned char *Buffer) {
    int i;

    printf("Format auto\n");
    Buffer[0] = 0xff;
    Buffer[1] = 0xff;
    for (i = 2; i < locDiskSectorSize; i++) {
        Buffer[i] = 0;
    }
    memset(locImage, 0, locDiskImageSize);
    return locDiskSectorSize;
}

/* Format image with medium density */
int Disk_FormatMedium(unsigned char *Buffer) {
    int i;

    printf("Format medium\n");
    locDiskTrackNum    = 0x28;
    locDiskSecPerTrack = 0x1a;
    locDiskSideNum     = 1;
    locDiskMode        = 0x04;
    locDiskSectorSize  = 0x80;

    /* copy to image header */
    locUpdateHeader();

    Buffer[0] = 0xff;
    Buffer[1] = 0xff;
    for (i = 2; i < locDiskSectorSize; i++) {
        Buffer[i] = 0;
    }
    memset(locImage, 0, locDiskImageSize);
    return locDiskSectorSize;
}

/* Prepare status information based on current configuratiion */
int Disk_GetStatus(unsigned char *Buffer) {
    printf("Get status\n");
    if (locDiskSecPerTrack == 0x12) {
        /* single or double density */
        if (locDiskSectorSize == 0x100) {
            Buffer[0] = 0x20;
        } else {
            Buffer[0] = 0x00;
        }
    } else {
        /* medium density */
        Buffer[0] = 0x80;
    }
    Buffer[1] = 0x20;
    Buffer[2] = 0xe0;
    Buffer[3] = 0x10;
    return 0;
}

/* Get disk configration (format info) */
int Disk_ReadPERCOM(unsigned char *Buffer) {
    printf("Read PERCOM\n");
    Buffer[0x00]  = locDiskTrackNum;
    Buffer[0x01]  = 0x00; /* Step rate 30ms */
    Buffer[0x02]  = locDiskSecPerTrack / 256;
    Buffer[0x03]  = locDiskSecPerTrack & 0xff;
    Buffer[0x04]  = locDiskSideNum - 1;
    Buffer[0x05]  = locDiskMode;
    Buffer[0x06]  = locDiskSectorSize / 256;
    Buffer[0x07]  = locDiskSectorSize & 0xff;
    Buffer[0x08]  = 0xff; /* Drive only */
    Buffer[0x09]  = 0x00;
    Buffer[0x0a]  = 0x00;
    Buffer[0x0b]  = 0x00;
    return 0;
}

/* Set disk configration (format info) */
int Disk_WritePERCOM(unsigned char *Buffer) {
    printf("Write PERCOM\n");

    locDiskTrackNum    = Buffer[0x00];
    locDiskSecPerTrack = 256 * Buffer[0x02] + Buffer[0x03];
    locDiskSideNum     = Buffer[0x04] + 1;
    locDiskMode        = Buffer[0x05];
    locDiskSectorSize  = 256 * Buffer[0x06] + Buffer[0x07];
    if ((locDiskTrackNum != 0x28) || (locDiskSideNum != 1)) {
        locCheckHeader();
        return -1;
    }
    /* copy to image header */
    locUpdateHeader();
    return 0;
}


/* Get sector from ATR image */
int Disk_ReadSector(int Num, unsigned char *Buffer) {
    int offset;
    int i;
    int size;

    printf("Read sector 0x%X\n", Num);
    offset = locSectorToImage(Num, &size);
    if (offset < 0) {
        return -1;
    }
    for (i = 0; i < size; i++) {
        Buffer[i] = locImage[offset + i];
    }
    return size;
}

/* Store sector in ATR image */
int Disk_WriteSector(int Num, unsigned char *Buffer) {
    int offset;
    int i;
    int size;

    printf("Write sector 0x%X\n", Num);
    offset = locSectorToImage(Num, &size);
    if (offset < 0) {
        return -1;
    }
    for (i = 0; i < size; i++) {
        locImage[offset + i] = Buffer[i];
    }
    return size;
}

/* Get size of a sector */
int Disk_GetSecSize(int Num) {
    int offset;
    int i;
    int size;

    offset = locSectorToImage(Num, &size);
    if (offset < 0) {
        return -1;
    }
    return size;
}

/* asioemu  - Atari 8bit SIO disk emulator
   siocmd.c - Atari SIO commands

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
#include <unistd.h>

/* Program include files */
#include "main.h"
#include "serial.h"
#include "siocmd.h"
#include "disk.h"

/* Local defines */
#define LOC_TIME_SECTOR_ACCESS    2 /* ms delay for feedback on sector access */
#define LOC_TIME_FORMAT        2000 /* ms delay for feedback on format */

/* Local types */
typedef struct {
    unsigned char Cmd;
    int (*Func)(int Atari);
} locSioCmd_t;

/* Local variables */
static char                 locSecBuf[0x200]; /* Sector buffer */
static const unsigned char *locSioCmd;        /* Pointer to SIO cmd in stream */

/* Support Functions */

/* Send ACK to Atari */
static void locAck(int Atari) {
    static const char ack[]="A";

    Serial_Write(Atari, (char*)ack, 1);
}

/* Send NACK to Atari */
static void locNack(int Atari) {
    static const char nack[]="N";

    Serial_Write(Atari, (char*)nack, 1);
}

/* Send COMPLETE to Atari */
static void locCompleted(int Atari) {
    static const char completed[]="C";

    Serial_Write(Atari, (char*)completed, 1);
}

/* Send ERROR to Atari */
static void locError(int Atari) {
    static const char error[]="E";

    Serial_Write(Atari, (char*)error, 1);
}

/* Wait MSec ms */
static void locSleep(int MSec) {
    usleep(MSec * 1000);
}


/* SIO Commands */

/* Format disk using current configuration */
static int locSioFormatAuto(int Atari) {
    int  i;
    unsigned char result[256];
    char checksum;
    int  size;

    locAck(Atari);
    size = Disk_FormatAuto(result);
    if (size < 0) {
        locError(Atari);
        return -1;
    }
    locSleep(LOC_TIME_FORMAT);
    locCompleted(Atari);
    Serial_Write(Atari, (char*)result, size);
    checksum = CalcCheckSum((char*)result, size);
    Serial_Write(Atari, &checksum, 1);
    return 0;
}

/* Format disk with medium density */
static int locSioFormatMedium(int Atari) {
    int  i;
    unsigned char result[256];
    char checksum;
    int  size;

    locAck(Atari);
    size = Disk_FormatMedium(result);
    if (size < 0) {
        locError(Atari);
        return -1;
    }
    locSleep(LOC_TIME_FORMAT);
    locCompleted(Atari);
    Serial_Write(Atari, (char*)result, size);
    checksum = CalcCheckSum((char*)result, size);
    Serial_Write(Atari, &checksum, 1);
    return 0;
}

/* Read controller status */
static int locSioStatus(int Atari) {
    unsigned char status[4];
    char checksum;
    int  x;

    locAck(Atari);
    x = Disk_GetStatus(status);
    if (x < 0) {
        locError(Atari);
        return -1;
    }
    locCompleted(Atari);
    Serial_Write(Atari, (char*)status, 4);
    checksum = CalcCheckSum((char*)status, 4);
    Serial_Write(Atari, &checksum, 1);
    return 0;
}

/* Get SIO speed */
static int locSioGetSpeed(int Atari) {
    static const char speed[] = {0x28};
    char checksum;

    locAck(Atari);
    locCompleted(Atari);
    Serial_Write(Atari, (char*)speed, 1);
    checksum = CalcCheckSum((char*)speed, 1);
    Serial_Write(Atari, &checksum, 1);
    return 0;
}

/* Get current disk configuration */
static int locSioReadPERCOM(int Atari) {
    unsigned char percom[12];
    char          checksum;
    int           x;

    locAck(Atari);
    x = Disk_ReadPERCOM(percom);
    if (x < 0) {
        locError(Atari);
        return -1;
    }
    locCompleted(Atari);
    Serial_Write(Atari, (char*)percom, 12);
    checksum = CalcCheckSum((char*)percom, 12);
    Serial_Write(Atari, &checksum, 1);
    printf("SIO READ Percomp:\n");
    HexDump(percom, 12);
    return 0;
}

/* Write current disk configuration */
static int locSioWritePERCOM(int Atari) {
    char percomp[12];
    char checksum, atarisum;
    int  len;
    int  sector;
    int  x;

    locAck(Atari);
    len = 0;
    while (len < 12) {
        len += Serial_Read(Atari, &percomp[len], 12 - len);
    }
    len = Serial_Read(Atari, &atarisum, 1);
    checksum = CalcCheckSum(percomp, 12);

    if (atarisum != checksum) {
       locNack(Atari);
       printf("WritePercomp checksum issue calc: %X vs received: %X\n", 
              checksum, atarisum);
       return -1;
    }
    locAck(Atari);
    locSleep(LOC_TIME_SECTOR_ACCESS);
    x = Disk_WritePERCOM(percomp);
    if (x < 0) {
        locError(Atari);
        return -1;
    }
    locCompleted(Atari);
    printf("SIO Write Percomp:\n");
    HexDump(percomp, 12);
    return 0;
}


/* Read sector */
static int locSioReadSec(int Atari) {
    char checksum;
    int  i;
    int  sector;
    int  size;

    locAck(Atari);

    sector = locSioCmd[2] | (locSioCmd[3] << 8);
    size = Disk_ReadSector(sector, locSecBuf);
    locSleep(LOC_TIME_SECTOR_ACCESS);
    if (size <= 0) {
        locError(Atari);
        return -1;
    }
    locCompleted(Atari);
    Serial_Write(Atari, locSecBuf, size);
    checksum = CalcCheckSum(locSecBuf, size);
    Serial_Write(Atari, &checksum, 1);
    return 0;
}

/* Write sector */
static int locSioWriteSec(int Atari) {
    char checksum, atarisum;
    int  len;
    int  sector;
    int  size;


    sector = locSioCmd[2] | (locSioCmd[3] << 8);
    size = Disk_GetSecSize(sector);
    if (size <= 0) {
        locNack(Atari);
        return -1;
    }
    locAck(Atari);

    len = 0;
    while (len < size) {
        len += Serial_Read(Atari, &locSecBuf[len], size - len);
    }
    len = Serial_Read(Atari, &atarisum, 1);
    checksum = CalcCheckSum(locSecBuf, size);

    if (atarisum != checksum) {
       locNack(Atari);
       printf("WriteSec checksum issue calc: %X vs received: %X\n", 
              checksum, atarisum);
       return -1;
    }
    locAck(Atari);
    Disk_WriteSector(sector, locSecBuf);
    locSleep(LOC_TIME_SECTOR_ACCESS);
    locCompleted(Atari);
    return 0;
}

/* Unkown SIO cmd */
static int locSioUnknown(int Atari) {
    printf("unknown command");
    HexDump((char*)locSioCmd, 5);
    printf("\n");
    locNack(Atari);
    return 0;
}

/* SIO command table */
const locSioCmd_t locCmdTable[] = {
    {0x21, locSioFormatAuto},
    {0x22, locSioFormatMedium},
    {'R',  locSioReadSec},
    {'S',  locSioStatus},
    {0x3F, locSioGetSpeed},
    {0x4E, locSioReadPERCOM},
    {0x4F, locSioWritePERCOM},
    {0x50, locSioWriteSec},
    {0x57, locSioWriteSec},
    {0xff, locSioUnknown}
};

/* Execute identified SIO command */
int SioCmd_Exec(int Atari, char *Cmd) {
    int err;
    int i;

    locSioCmd = Cmd;
    i = 0;
    while (locCmdTable[i].Cmd != 0xff) {
        if (locSioCmd[1] == locCmdTable[i].Cmd) {
            break;
        }
        i++;
    }
    err = locCmdTable[i].Func(Atari);
    return err;
}

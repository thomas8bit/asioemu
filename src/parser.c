/* asioemu  - Atari 8bit SIO disk emulator
   parser.c - Parse SIO commands

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

/* Program include files */
#include "main.h"
#include "parser.h"
#include "siocmd.h"

/* Local defines */
#define LOC_STATE_IDLE            0    /* Waiting for command */
#define LOC_STATE_ID_DETECTED     1    /* Own id detected */
#define LOC_STATE_SIOCMD_RECEIVED 2    /* Complete command receive */
#define LOC_STATE_SIOCMD_VERIFIED 3    /* Command verified (checksum) */

/* Local variables */
static char locSioCmd[5];               /* SIO command buffer */
static char locSioIndex;                /* Receive SIO buffer index */
static char locState = LOC_STATE_IDLE;  /* Parser status */


/* Look for own id in data stream */
static int locFindMe(char Device, char *Buffer, int Offs, int Size) {
    int i;

    i = Offs;
    while (i < Size) {
        if (Buffer[i] == Device) {
            locState     = LOC_STATE_ID_DETECTED;
            locSioIndex = 0;
            return i;
        }
        i++;
    }
    return -1;
}

/* Get command data & checksum */
static int locGetSioCmd(char *Buffer, int Offset, int Size) {
    int i;

    while ((Offset < Size) && (locState == LOC_STATE_ID_DETECTED)) {
        locSioCmd[locSioIndex] = Buffer[Offset];
        Offset++;
        locSioIndex++;
        if (locSioIndex >= 5) {
            locState     = LOC_STATE_SIOCMD_RECEIVED;
        }
    }
    return Offset;
}

/* Calculate own checksum & verify with received sum */
static int locVerifyChecksum(void) {
    char checksum;

    locState = LOC_STATE_IDLE;
    checksum = CalcCheckSum(locSioCmd, 4);
    if (checksum == locSioCmd[4]) {
        locState = LOC_STATE_SIOCMD_VERIFIED;
        return 1;
    }
    printf("Checksum issue calc: %X vs received: %X\n", checksum,
           locSioCmd[4]);
    locState = LOC_STATE_IDLE;
    return 0;
}


/* Parse serial data stream */
int Parser_Exec(int Atari, char Device, char *InBuf, int InSize) {
    static const char nack[]="N";
    int offs;
    int x;

    offs = 0;
    while ((locState >= LOC_STATE_SIOCMD_RECEIVED)
           || ((offs < InSize) && (offs >= 0))) {
        switch (locState) {
        case LOC_STATE_IDLE:
            offs = locFindMe(Device, InBuf, offs, InSize);
            break;
        case LOC_STATE_ID_DETECTED:
            offs = locGetSioCmd(InBuf, offs, InSize);
            break;
        case LOC_STATE_SIOCMD_RECEIVED:
            x = locVerifyChecksum();
            if (x == 0) {
                Serial_Write(Atari, nack, 1);
                return -1;
            }
            break;
        case LOC_STATE_SIOCMD_VERIFIED:
            SioCmd_Exec(Atari, locSioCmd);
            locState = LOC_STATE_IDLE;
            break;
        }
    }
    return 0;
}

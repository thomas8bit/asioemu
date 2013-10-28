/* asioemu - Atari 8bit SIO disk emulator
   main.c  - Program startup and mainloop

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
#include <getopt.h>
#include <string.h>
#include <termios.h>

/* Program include files */
#include "main.h"
#include "serial.h"
#include "parser.h"
#include "disk.h"

/* Local variables of this module */
static char *locDevice;       /* Serial port for communication with Atari*/
static char  locSioId;        /* Disk ID of emulated device '1'..'4' */
static char *locDiskImage;    /* Name of disk image file */
static char  locDiskCreate;   /* Flag for creating image instead of reading it */
static char  locLogOnly;      /* Flag for communication LOG only (no emu) */
static char  locInBuf[1000];  /* Receive buffer for data from Atari */
static char  locOutBuf[1000]; /* Transmit buffer for data to Atari */


/* Get single character directly from keyboard w/o blocking */
static int locGetKey() {
    int character;
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    character = fgetc(stdin);

    /* restore the original terminal attributes */
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

    return character;
}


/* Print the mainloop menu */
static void locPrintMenu(void) {
    printf("q - Quit, w - Write image -->");
}

/* Program's main loop */
static int locMainLoop(void) {
    int atari;
    int len;
    int key;
    int x;

    /* Load or create disk image file */
    if (locDiskCreate != 0) {
        len = Disk_Create(locDiskCreate, locDiskImage);
    } else {
        len = Disk_Load(locDiskImage);
    }
    if (len < 0) {
        Error(ERR_DISKIMG);
        return -1;
    }

    /* Setup serial communication with Atari */
    atari = Serial_Open(locDevice);
    if (atari < 0) {
        Error(ERR_UART);
        return -1;
    }
    len = 0;

    /* main loop */
    locPrintMenu();
    do {
        /* Get raw serial data from Atari */
        len = Serial_Read(atari, locInBuf, sizeof(locInBuf));
        if (len > 0) {
            printf("\n");
            /* Dump or parse (& reply to) the data from Atari */
            if (locLogOnly) {
                HexDump(locInBuf, len);
            } else {
                Parser_Exec(atari, locSioId, locInBuf, len);
            }
            locPrintMenu();
        }
        /* Get and execute keyboard commands */
        key = locGetKey();
        if (key > 0) {
            printf("\n");
            switch (key) {
            case 'w':
                Disk_Save(locDiskImage);
                break;
            default:
                Serial_Close(atari);
                return 0;
                break;
            }
        }
    } while (1);
    Serial_Close(atari);
    return 0;
}

/* Print usage information */
static void locHelp(void) {
    printf("options:\n");
    printf("  -h               : Print help.\n");
    printf("  -d <device>      : Set UART device (default /dev/ttyS0).\n");
    printf("  -s <sioid>       : Select SIO identifier (default 2).\n");
    printf("  -i <atr image>   : Name of ATR image file.\n");
    printf("  -c <s|m|d>       : Create <atr_iame> single/medium/double density.\n");
    printf("  -l               : Only log communication.\n");
}

/* Setup default configuration */
static void locSetupDefault(void) {
    locDevice = "/dev/ttyS0";
    locSioId ='2';
    locDiskImage  = "dummy.atr";
    locDiskCreate = 0;
    locLogOnly    = 0;
}

/* Calculate Atari like checksum */
char CalcCheckSum(char *Buffer, int Size) {
    int  i;
    unsigned char checksum;
    unsigned int rawsum;
    unsigned char oldsum;

    rawsum = 0;
    checksum = 0;
    for (i = 0; i < Size; i++) {
        oldsum = checksum;
        checksum += (unsigned char)Buffer[i];
        if (oldsum > checksum) {
            checksum++;
        }
    }
    return checksum;
}

/* Output hexdump of a buffer */
void HexDump(char *Buffer, int Size) {
    int i;
    
    for (i = 0; i < Size; i++) {
        if ((i % 16) == 0) {
            printf("\n");
        }
        printf("%2.2X ", Buffer[i] & 0xff);
    }
    printf("\n");
}

/* Central error function */
void Error(int Err) {
    printf("ERROR: ");
    switch (Err) {
    case ERR_PARAM:
        printf("Wrong parameter.");
        break;
    case ERR_DISKIMG:
        printf("Loading disk image failed.");
        break;
    default:
        printf("Unknown");
        break;
    }
    printf("\n");
}


/* Parse options & start main loop */
int main(int argc, char *argv[]) {
    int opt;

    printf("asioemu version %s - Atari 8-bit Disk Emulator \n", VERSION);
    locSetupDefault();
    do {
        opt = getopt(argc, argv, "?hd:s:i:c:l");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 'd':
            locDevice = optarg;
            break;
        case 'i':
            locDiskImage = optarg;
            break;
        case 's':
            locSioId = *optarg;
            break;
        case 'c':
            locDiskCreate = *optarg;
            break;
        case 'l':
            locLogOnly = 1;
            break;
        case 'h':
        case '?':
        case ':':
            locHelp();
            return 0;
            break;
        }

    } while (1);
    if (optind < argc) {
        Error(ERR_PARAM);
        return -1;
    }
    locMainLoop();
    return 0;
}

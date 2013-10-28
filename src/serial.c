/* asioemu  - Atari 8bit SIO disk emulator
   serial.c - Serial communication

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
#include <fcntl.h>              /* File control definitions */
#include <termios.h>            /* POSIX terminal control definitions */
#include <sys/ioctl.h>

/* Program include files */
#include "serial.h"

/* Open and configure serial line */
int Serial_Open(char *Name) {
    int fd;
    int modelines = 0;
    struct termios options;

    fd = open (Name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        return -1;
    }
    if (!isatty (fd)) {
        return -1;
    }
    /* get the current options */
    fcntl (fd, F_SETFL, 0);
    if (tcgetattr (fd, &options) != 0) {
        return -1;
    }
    /* set 19200 baud */
    cfsetispeed (&options, B19200);
    cfsetospeed (&options, B19200);

    /* configure cflag options */
    options.c_cflag &= ~PARENB; /* no parity */
    options.c_cflag &= ~CSTOPB; /* 1 stop bit */
    options.c_cflag &= ~CSIZE;  /* 8 data bits */
    options.c_cflag |= CS8; 
    options.c_cflag |= (CLOCAL | CREAD); /* ignore CD signal*/

    /* configure lflag options */
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* configure oflag options */
    options.c_oflag &= ~OPOST;  /* "raw" input */

    /* configure cc field */
    options.c_cc[VMIN] = 0;     /* wait for min 0 bytes */
    options.c_cc[VTIME] = 10;   /* timeour 10s */

    /* configure iflag options */
    options.c_iflag &= ~(BRKINT | PARMRK |  INPCK | ISTRIP
                       | INLCR | IGNCR | ICRNL | IUCLC | IXON | IXANY
                       | IXOFF | IMAXBEL | IUTF8);
    options.c_iflag |= (IGNBRK | IGNPAR);

    /* Flush and set options */
    tcflush (fd, TCIOFLUSH);
    if (-1 == tcsetattr (fd, TCSAFLUSH, &options) != 0) {
        return -1;
    }

    /* Set IO-Control-Parameter and check the device. */
    if (-1 == ioctl (fd, TIOCEXCL, &modelines)) {
        return -1;
    }
    return fd;
}

/* Close serial line */
int Serial_Close(int File) {
    int err;
    
    err = close(File);
    return err;
}

/* Read from serial line */
int Serial_Read(int File, char *Buffer, int MaxLen) {
    int len;

    len = read(File, Buffer, MaxLen);
    return len;
}

/* Write to serial line */
int Serial_Write(int File, char *Buffer, int Len) {
    int len;

    len = write(File, Buffer, Len);
    return len;
}

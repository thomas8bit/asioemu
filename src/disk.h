/* asioemu - Atari 8bit SIO disk emulator
   disk.h  - Interface of disk module

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

#ifndef DISK_H
#define DISK_H

#ifdef __cplusplus
extern "C" {
#endif

int Disk_Create(char Format, char *Name);
int Disk_Load(char *Name);
int Disk_Save(char *Name);
int Disk_FormatAuto(unsigned char *Buffer);
int Disk_FormatMedium(unsigned char *Buffer);
int Disk_GetStatus(unsigned char *Buffer);
int Disk_ReadPERCOM(unsigned char *Buffer);
int Disk_WritePERCOM(unsigned char *Buffer);
int Disk_ReadSector(int Num, unsigned char *Buffer);
int Disk_WriteSector(int Num, unsigned char *Buffer);
int Disk_GetSecSize(int Num);

#ifdef __cplusplus
}
#endif

#endif

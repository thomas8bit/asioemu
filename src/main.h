/* asioemu - Atari 8bit SIO disk emulator
   main.h  - Interface of  main module

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

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define ERR_PARAM     -10
#define ERR_UART      -20
#define ERR_DISKIMG   -30

void Error(int Err);
char CalcCheckSum(char *Buffer, int Size);
void HexDump(char *Buffer, int Size);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */

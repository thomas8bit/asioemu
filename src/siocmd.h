/* asioemu  - Atari 8bit SIO disk emulator
   siocmd.h - Interface of siocmd module

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

#ifndef SIOCMD_H
#define SIOCMD_H

#ifdef __cplusplus
extern "C" {
#endif

int SioCmd_Exec(int Atari, char *Cmd);

#ifdef __cplusplus
}
#endif

#endif /* SIOCMD_H */

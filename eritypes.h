/*
    ERI(Entis Rasterized Image) data defination using glib
    Copyright (C) 2000  SAKAI Masahiro

        SAKAI Masahiro                  <ZVM01052@nifty.ne.jp>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef ERITYPES_H_INCLUDED
#define ERITYPES_H_INCLUDED

#include <glib-2.0/glib.h>

typedef void    *PVOID;
typedef gint    INT,   *PINT;
typedef guint   UINT,  *PUINT;
typedef glong   LONG,  *PLONG;
typedef gulong  ULONG, *PULONG;

typedef guchar  BYTE,   *PBYTE;
typedef guint16 WORD,   *PWORD;
typedef gint16  SWORD,  *PSWORD;
typedef guint32 DWORD,  *PDWORD;
typedef gint32  SDWORD, *PSDWOED;
typedef guint64 QWORD,  *PQWORD;
typedef gint64  SQWORD, *PSQWORD;

/* FIXME: Does it work on platforms other than ia32? */
typedef gfloat  REAL32, *PREAL32;

#endif

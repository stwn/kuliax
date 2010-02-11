/*
 *  main.h, gKamus (http://gkamus.sourceforge.net)
 *  Copyright (C) 2008-2009, Ardhan Madras <ajhwb@knac.com>
 *
 *  gKamus is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  gKamus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gKamus; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <gtk/gtk.h>

typedef struct
{
  gchar *word;
  gchar *definition;
} Dictionary;

typedef enum
{
  GKAMUS_MODIFY_ADD = 0,
  GKAMUS_MODIFY_EDIT,
} GkamusModifyMode;

#ifdef G_OS_UNIX
typedef enum
{
  GKAMUS_SHORTCUT_DESKTOP = 0,
  GKAMUS_SHORTCUT_MENU,
} GkamusShortcutMode;
#endif

typedef enum
{
  GKAMUS_DIC_EN = 0,
  GKAMUS_DIC_ID
} GkamusDicMode;

typedef enum
{
  GKAMUS_DIC_STATUS_ERROR = 0,
  GKAMUS_DIC_STATUS_NOT_FOUND,
  GKAMUS_DIC_STATUS_INVALID,
  GKAMUS_DIC_STATUS_MAX_ENTRY,
  GKAMUS_DIC_STATUS_OK
} GkamusDicStatus;

#define PROGRAM_NAME    "gKamus"
#define	PROGRAM_VERS	  "0.3"
#define PROGRAM_DESC    "Kamus Bahasa Inggris - Indonesia"
#define PROGRAM_SITE    "http://gkamus.sourceforge.net"
#define PROGRAM_ICON    "gkamus.png"
#define MAX_WORD        16
#define MAX_DEFINITION  256
#define MAX_ENTRY       100000

extern GList *dict;
extern gchar *dict_file;
extern const gchar const * dict_en;
extern const gchar const * dict_id;
extern gchar *prg_name;
extern const gchar * const dic_errmsg[];

#endif /* __MAIN_H__ */

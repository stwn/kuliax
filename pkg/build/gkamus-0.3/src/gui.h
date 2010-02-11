/*
 *  gui.h, gKamus (http://gkamus.sourceforge.net)
 *  Copyright (C) 2008-2009, Ardhan Madras <ajhwb@knac.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  File ini adalah bagian dari gKamus http://gkamus.sourceforge.net
 *  Copyright(c) 2008-2009, Ardhan Madras <ajhwb@knac.com>
 */

#ifndef __GUI_H__
#define __GUI_H__

#include <gtk/gtk.h>

#include "main.h"

enum
{
  COLUMN_WORD,
  COLUMN_N
};

extern GtkWidget *window_main;
extern GtkWidget *menu_save;
extern GtkWidget *menu_tool_ei;
extern GtkWidget *menu_tool_ie;
extern GtkWidget *entry_search;
extern GtkWidget *treev_word;
extern GtkWidget *textv_definition;
extern GtkWidget *window_modify;
extern GtkWidget *entry_mod;
extern GtkWidget *textv_mod;
extern GtkWidget *window_alpha;
extern GtkWidget *window_verb;
extern GtkWidget *treev_verb;

void set_sensitive (GtkWidget*, gboolean);
gint create_dialog (GtkMessageType, gpointer, const gchar*, const gchar*, ...);
GtkWidget* create_window_main (void);
GtkWidget* create_window_modify (GkamusModifyMode);
GtkWidget* create_window_alpha (void);
GtkWidget* create_window_verb (void);
GtkWidget* create_dialog_about (void);
GtkWidget* create_dialog_file (gpointer, GtkFileChooserAction);

#endif /* __GUI_H__ */


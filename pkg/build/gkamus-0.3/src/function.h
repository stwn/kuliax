/*
 *  function.h, gKamus (http://gkamus.sourceforge.net)
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

#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <gtk/gtk.h>

#include "main.h"

gsize write_dict (const gchar*);
gint search_dictb (const gchar*);
gint search_dict (const gchar*);
gint save_dict_func (const gchar*, gchar*, gint);
gint check_dict_file (gpointer);
gint list_compare (const Dictionary*, const Dictionary*);
void free_elements (Dictionary*, gpointer);
GkamusDicStatus load_dict (const gchar*);
void set_treev_word (void);
void show_dicstatus (GkamusDicStatus);

#ifdef G_OS_UNIX
gint generate_shortcut (GkamusShortcutMode);
#endif

#endif /* __FUNCTION_H__ */

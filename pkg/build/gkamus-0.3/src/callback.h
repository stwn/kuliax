/*
 *  callback.h, gKamus (http://gkamus.sourceforge.net)
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

#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include <gtk/gtk.h>

gboolean on_window_main_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_entry_search_changed (GtkWidget*, gpointer);
void on_window_modify_destroy (GtkWidget*, gpointer);
void on_button_mod_ok_clicked (GtkWidget*, gpointer);
gboolean on_textv_mod_key_press_event (GtkWidget*, GdkEventKey*, gpointer);
void on_textv_mod_buffer_changed (GtkTextBuffer*, gpointer);
void select_treev_word (gint);
void on_menu_save_activate (GtkWidget*, gpointer);
void on_menu_saveas_activate (GtkWidget*, gpointer);
void on_menu_quit_activate (GtkWidget*, gpointer);
void on_menu_copy_activate (GtkWidget*, gpointer);
void on_menu_paste_activate (GtkWidget*, gpointer);
void on_menu_add_activate (GtkWidget*, gpointer);
void on_menu_edit_activate (GtkWidget*, gpointer);
void on_menu_delete_activate (GtkWidget*, gpointer);
void on_menu_tool_ei_activate (GtkWidget*, gpointer);
void on_menu_tool_ie_activate (GtkWidget*, gpointer);
void on_menu_alpha_activate (GtkWidget*, gpointer);
void on_menu_verb_activate (GtkWidget*, gpointer);
void on_menu_about_activate (GtkWidget *, gpointer);
#ifdef G_OS_UNIX
void on_menu_short_desktop_activate (GtkWidget *, gpointer);
void on_menu_short_menu_activate (GtkWidget *, gpointer);
#endif
void on_treev_word_columns_changed (GtkWidget *, gpointer);
void on_button_paste_clicked (GtkWidget *, gpointer);
void on_button_find_clicked (GtkWidget *, gpointer);
gboolean on_window_alpha_delete_event (GtkWidget*, GdkEvent*, gpointer);
void on_window_verb_destroy (GtkWidget*, gpointer);

#endif /* __CALLBACK_H__ */


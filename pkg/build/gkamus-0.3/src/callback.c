/*
 *  callback.c, gKamus (http://gkamus.sourceforge.net)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "gui.h"
#include "callback.h"
#include "function.h"

static void modified_notify (gboolean);
static void change_cursor (void);
static gboolean restore_cursor (gpointer);
static gboolean set_sensitive_timer (GtkWidget*);
static gboolean change_dict (gpointer);
static gboolean modified = FALSE;
static gboolean eng = TRUE; /* shit! why we need this? */

#if 0
gint
column_sort_func (GtkTreeModel *model, 
                  GtkTreeIter  *a, 
                  GtkTreeIter  *b,
                  gpointer      data)
{
  gchar *str1, *str2;
  gint ret;
    
  gtk_tree_model_get (model, a, COLUMN_WORD, &str1, -1);
  gtk_tree_model_get (model, b, COLUMN_WORD, &str2, -1);
  ret = strcmp (str1, str2);
  g_free (str1);
  g_free (str2);

  return ret;
}
#endif

/*
 * Notify perubahan pada title window_main.
 */
static void
modified_notify (gboolean state)
{
  gchar *title;
    
  modified = state;
  title = g_strdup_printf ("%s%s", state ? "*" : "", PROGRAM_NAME);
  gtk_window_set_title ((GtkWindow *) window_main, title);
  set_sensitive (menu_save, state);
  g_free (title);
}

/*
 * Set sensitive widget.
 */
static gboolean
set_sensitive_timer (GtkWidget *widget)
{
  set_sensitive (widget, !GTK_WIDGET_IS_SENSITIVE (widget));
  return FALSE;
}

/*
 * Ganti cursor ke mode "busy".
 */
static void
change_cursor (void)
{
  GdkScreen *screen;
  GdkWindow *window;
  GdkCursor *cursor;

  screen = gtk_window_get_screen ((GtkWindow *) window_main);
  window = gdk_screen_get_root_window (screen);
  cursor = gdk_cursor_new (GDK_WATCH); /* lihat gdkcursor.h */

  gdk_window_set_cursor (window, cursor);
  gdk_cursor_unref (cursor);
}

/*
 * Kembalikan cursor ke normal.
 */
static gboolean
restore_cursor (gpointer data)
{
  GdkScreen *screen;
  GdkWindow *window;
  GdkCursor *cursor;

  screen = gtk_window_get_screen ((GtkWindow *) window_main);
  window = gdk_screen_get_root_window (screen);
  cursor = gdk_cursor_new (GDK_LEFT_PTR);

  gdk_window_set_cursor (window, cursor);
  gdk_cursor_unref (cursor);
  return FALSE;
}

gboolean
on_window_main_delete_event (GtkWidget *widget, 
                             GdkEvent  *event, 
                             gpointer   data)
{
  if (modified)
    {
      gint d = create_dialog (GTK_MESSAGE_QUESTION, window_main, "Keluar",
                              "Kamus belum disimpan, keluar sekarang juga?");
      if (d != GTK_RESPONSE_OK)
        return TRUE; /* propagate */
    }
  gtk_main_quit ();
  return TRUE; /* agar window_main tdk di destroy */
}

/*
 * Cari kata yang paling mendekati entry.
 */
void
on_entry_search_changed (GtkWidget *entry,
                         gpointer   data)
{
  gchar *word;
  gint len;

  word = g_strdup (gtk_entry_get_text ((GtkEntry *) entry));
  word = g_strstrip (word);
  len = strlen (word);
  if (!len)
    {
      g_free (word);
      return;
    }

  GList *list = dict;
  Dictionary *d;
  gint id;

  while (list)
    {
      d = list->data;
      if (!g_ascii_strncasecmp (word, d->word, len))
        {
          id = g_list_index (dict, d);
          select_treev_word (id);
          break;
        }
      list = list->next;
    }
  g_free (word);
}

void
on_window_modify_destroy (GtkWidget *widget, 
                          gpointer   data)
{
  gtk_widget_destroy (window_modify);
}

/*
 * Tambah / Edit entry.
 */
void
on_button_mod_ok_clicked (GtkWidget *widget, 
                          gpointer   data)
{
  gint i = GPOINTER_TO_INT (data);
  g_return_if_fail (i == GKAMUS_MODIFY_ADD || i == GKAMUS_MODIFY_EDIT);

  GtkTextBuffer *buf;
  GtkTextIter start, end;
  gchar *tmp;
  gchar word[MAX_WORD + 1], def[MAX_DEFINITION + 1];

  tmp = g_ascii_strdown (gtk_entry_get_text ((GtkEntry *) entry_mod), -1);
  tmp = g_strstrip (tmp);
  g_snprintf (word, sizeof(word), tmp);
  g_free (tmp);

  buf = gtk_text_view_get_buffer ((GtkTextView *) textv_mod);
  /* MAX_DEFINITION dicek di callback signal "changed" buffer */
  gtk_text_buffer_get_bounds (buf, &start, &end);
  tmp = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
  tmp = g_strstrip (tmp);
  /* lihat on_textv_mod_key_press_event() */
  gtk_text_buffer_set_text (buf, tmp, -1);
  g_snprintf (def, sizeof(def), tmp);
  g_free (tmp);

  /* cek input */
  if (!strlen (word) || !strlen (def))
    {
      create_dialog (GTK_MESSAGE_WARNING, window_modify,
                     i == GKAMUS_MODIFY_ADD ? "Tambah Kata" : "Edit Kata",
                     "Silahkan periksa kembali entri yang dimasukkan.");
      return;
    }

  gint s = search_dict (word);
  if (s != -1 && i == GKAMUS_MODIFY_ADD)
    {
      create_dialog (GTK_MESSAGE_WARNING, window_modify, "Tambah Kata",
                     "Kata \"%s\" telah ada dalam database kamus.", word);
      return;
    }

  Dictionary *old_dic;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint *id;

  /* cari data list sesuai indeks treeview */
  selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_word);
  gtk_tree_selection_get_selected (selection, &model, &iter); /* selalu TRUE */
  path = gtk_tree_model_get_path (model, &iter);
  id = gtk_tree_path_get_indices (path);
  old_dic = g_list_nth_data (dict, *id);
  gtk_tree_path_free (path);

  /* return jika tidak ada perubahan */
  if (!strcmp (old_dic->word, word) && !strcmp (old_dic->definition, def))
    {
      gtk_widget_destroy (window_modify);
      return;
    }
  /* jangan cek jika @word adalah kata yang dipilih di treeview */
  if (i == GKAMUS_MODIFY_EDIT && (s != -1 && strcmp (old_dic->word, word)))
    {
      create_dialog (GTK_MESSAGE_WARNING, window_modify, "Edit Kata",
                     "Kata \"%s\" telah ada dalam database kamus.", word);
     return;
    }

  if (i == GKAMUS_MODIFY_ADD)
    {
      Dictionary *new_dic;

      /* tambah list @dict */
      new_dic = g_new (Dictionary, 1);
      new_dic->word = g_strdup (word);
      new_dic->definition = g_strdup (def);
      dict = g_list_prepend (dict, new_dic);
      /* re-sort @dict */
      set_sensitive (window_main, FALSE);
      dict = g_list_sort (dict, (GCompareFunc) list_compare);
      set_treev_word ();
      select_treev_word (g_list_index (dict, new_dic));
      g_idle_add ((GSourceFunc) set_sensitive_timer, window_main);
      g_print ("added: %s\nindices = %i\ntotal: %i\n\n", new_dic->word,
               g_list_index (dict, new_dic), g_list_length (dict));
    }
  else if (i == GKAMUS_MODIFY_EDIT)
    {
      tmp = g_strdup (old_dic->word);
      g_free (old_dic->word);
      g_free (old_dic->definition);
      old_dic->word = g_strdup (word);
      old_dic->definition = g_strdup (def);

      /* jika kata berubah, sort list dan load kembali */
      if (strcmp (old_dic->word, tmp))
        {
          dict = g_list_sort (dict, (GCompareFunc) list_compare);
          set_sensitive (window_main, FALSE);
          set_treev_word ();
          g_idle_add ((GSourceFunc) set_sensitive_timer, window_main);
          select_treev_word (g_list_index (dict, old_dic));
        }
      else
        {
          buf = gtk_text_view_get_buffer ((GtkTextView *) textv_definition);
          gtk_text_buffer_set_text (buf, old_dic->definition, -1);
        }
      g_print ("edited: %s -> %s\nindices = %i\ntotal: %i\n\n", tmp, old_dic->word,
               g_list_index (dict, old_dic), g_list_length (dict));
      g_free (tmp);
    }
  modified_notify (TRUE);
  gtk_widget_destroy (window_modify);
}

/*
 * Signal "key-press-event textv_mod.
 */
gboolean
on_textv_mod_key_press_event (GtkWidget   *widget,
                              GdkEventKey *event,
                              gpointer     data)
{
  if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
    {
      gtk_widget_grab_focus ((GtkWidget *) data);
      gtk_widget_activate ((GtkWidget *) data);
    }
  return FALSE;
}

/* 
 * Limit buffer definition, MAX_DEFINITION. 
 */
void
on_textv_mod_buffer_changed (GtkTextBuffer *buffer, 
                             gpointer       data)
{
  GtkTextIter start, end;
  gchar *tmp;
    
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  tmp = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  if (strlen (tmp) > MAX_DEFINITION)
    gtk_text_buffer_set_text (buffer, tmp, MAX_DEFINITION);
  g_free (tmp);
}

/* 
 * Paste clipboard ke entry_search. 
 */
static void
paste_to_entry (GtkClipboard *clipboard, 
                const gchar  *buffer, 
                gpointer      data)
{
  if (buffer)
    gtk_entry_set_text ((GtkEntry *) entry_search, buffer);
}

/* 
 * Seleksi di treev_word. okay, you keep me busy. 
 */
void
select_treev_word (gint indices)
{
  GtkTreeSelection *selection;
  GtkTreePath *path;

  selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_word);
  path = gtk_tree_path_new_from_indices (indices, -1);
  gtk_tree_selection_select_path (selection, path);
  /* set cursor */
  gtk_tree_view_scroll_to_cell ((GtkTreeView *) treev_word, path, NULL, TRUE, 0, 0);
  gtk_tree_view_set_cursor ((GtkTreeView *) treev_word, path, NULL, FALSE);
  gtk_tree_path_free (path);
}

/* 
 * Menu File -> Simpan. 
 */
void
on_menu_save_activate (GtkWidget *widget, 
                       gpointer   data)
{
  if (!modified)
    return;
  if (!write_dict (dict_file))
    {
      create_dialog (GTK_MESSAGE_ERROR, window_main, "Simpan Kamus",
                     "Kamus tidak dapat disimpan, periksa permisi anda.");
      return;
    }
  modified_notify (FALSE);
}

/* 
 * Menu File -> Simpan sbg. 
 */
void
on_menu_saveas_activate (GtkWidget *widget, 
                         gpointer   data)
{
  GtkWidget *dialog;
  gchar *tmp;

  dialog = create_dialog_file (window_main, GTK_FILE_CHOOSER_ACTION_SAVE);
  tmp = g_path_get_basename (dict_file);
  gtk_file_chooser_set_current_name ((GtkFileChooser *) dialog, tmp);
  
  gint d = gtk_dialog_run ((GtkDialog *) dialog);
  if (d != GTK_RESPONSE_OK)
    {
        gtk_widget_destroy (dialog);
        return;
    }
  gchar *file = gtk_file_chooser_get_filename ((GtkFileChooser *) dialog);
  if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
      gtk_widget_destroy (dialog);
      gint r = create_dialog (GTK_MESSAGE_QUESTION, window_main, 
                              "Simpan Kamus", "File \"%s\" telah ada, tulis "
                              "ulang file ini?", file);
      if (r != GTK_RESPONSE_OK)
        {
          g_free (file);
          return;
        }
    }
  if (!write_dict (file))
    {
     gtk_widget_destroy (dialog);
     create_dialog (GTK_MESSAGE_ERROR, window_main, "Simpan Kamus",
                    "Kamus tidak dapat disimpan, periksa permisi anda.");
    }
  g_free (file);
  gtk_widget_destroy (dialog);
}

/* 
 * Menu File -> Keluar. 
 */
void
on_menu_quit_activate (GtkWidget *widget,
                       gpointer   data)
{
  on_window_main_delete_event (NULL, NULL, NULL);
}

/*
 * Menu Edit -> Copy, copy kata ke clipboard. 
 */
void 
on_menu_copy_activate (GtkWidget *widget, 
                       gpointer   data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter	iter;
    
  selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_word);
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      create_dialog (GTK_MESSAGE_WARNING, window_main, "Copy Kata",
                     "Silahkan pilih kata yang akan di copy.\n");
      return;
    }

  GtkClipboard *clipboard;
  gchar *buffer;
    
  gtk_tree_model_get (model, &iter, COLUMN_WORD, &buffer, -1);
  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  /* copy buffer ke clipboard */
  gtk_clipboard_set_text (clipboard, buffer, -1);
  g_free (buffer);
}

/* 
 * Menu Edit -> Paste, set text ke entry_search dari clipboard. 
 */
void 
on_menu_paste_activate (GtkWidget *widget,
                        gpointer   data)
{
  GtkClipboard *clipboard;

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_request_text (clipboard, paste_to_entry, data);
}

/* 
 * Menu Edit -> Tambah. 
 */
void
on_menu_add_activate (GtkWidget *widget, 
                      gpointer   data)
{
  window_modify = create_window_modify (GKAMUS_MODIFY_ADD);
  gtk_widget_show (window_modify);
}

/* 
 * Menu Edit -> Edit. 
 */
void
on_menu_edit_activate (GtkWidget *widget, 
                       gpointer   data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean selected;
    
  model = gtk_tree_view_get_model ((GtkTreeView *) treev_word);
  selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_word);
  selected = gtk_tree_selection_get_selected (selection, &model, &iter);
  if (!selected)
    {
      create_dialog (GTK_MESSAGE_WARNING, window_main, "Edit Kata",
                     "Silahkan pilih kata yang akan diedit.");
      return;
    }
    
  Dictionary *dic;
  GtkTextBuffer *buf;
  GtkTreePath *path;
  gint *id;
    
  path = gtk_tree_model_get_path (model, &iter);
  id = gtk_tree_path_get_indices (path);
  dic = g_list_nth_data (dict, *id);
    
  window_modify = create_window_modify (GKAMUS_MODIFY_EDIT);
  gtk_entry_set_text ((GtkEntry *) entry_mod, dic->word);
  buf = gtk_text_view_get_buffer ((GtkTextView *) textv_mod);
  gtk_text_buffer_set_text (buf, dic->definition, -1);
  gtk_text_view_set_buffer ((GtkTextView *) textv_mod, buf);
  gtk_widget_show (window_modify);
  gtk_tree_path_free (path);
}

/* 
 * Menu Edit -> Hapus. 
 */
void
on_menu_delete_activate (GtkWidget *widget, 
                         gpointer   data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean selected;  
    
  model = gtk_tree_view_get_model ((GtkTreeView *) treev_word);
  selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_word);
  selected = gtk_tree_selection_get_selected (selection, &model, &iter);
  if (!selected)
    {
      create_dialog (GTK_MESSAGE_WARNING, window_main, "Hapus Kata",
                     "Silahkan pilih kata yang akan dihapus.");
      return;
    }

  Dictionary *dic;
  GtkTreePath *path;
  gint *id, ret;

  path = gtk_tree_model_get_path (model, &iter);
  id = gtk_tree_path_get_indices (path);
  dic = g_list_nth_data (dict, *id);

  ret = create_dialog (GTK_MESSAGE_QUESTION, window_main, "Hapus Kata", "Hapus "
                       "entri kata \"%s\" dari database kamus?", dic->word);
  if (ret != GTK_RESPONSE_OK)
    {
      gtk_tree_path_free (path);
      return;
    }
  dict = g_list_remove (dict, dic);
  gtk_list_store_remove ((GtkListStore *) model, &iter);
  g_print ("deleted: %s\nindices = %i\ntotal: %i\n\n", 
           dic->word, *id, g_list_length (dict));
  /* ganti row */
  select_treev_word ((*id == g_list_length (dict)) ? 0 : *id);
  modified_notify (TRUE);
  g_free (dic->word);
  g_free (dic->definition);
  g_free (dic);
  gtk_tree_path_free (path);
}

/*
 * Timer ganti kamus.
 */
static gboolean
change_dict (gpointer data)
{
  gchar *file;
  gint mode;
  gint status;

  mode = GPOINTER_TO_INT (data);
  g_return_val_if_fail (mode != GKAMUS_DIC_EN || mode != GKAMUS_DIC_ID, FALSE);

  file = (mode == GKAMUS_DIC_EN) ? (gchar *) dict_en : (gchar *) dict_id;

  status = load_dict (file);
  if (status != GKAMUS_DIC_STATUS_OK)
    {
      restore_cursor (NULL);
      g_critical ("%s.\n", dic_errmsg[status]);
      show_dicstatus (status);
      gtk_main_quit ();
    }
  else
    {
      set_treev_word ();
      select_treev_word (0);
    }
  eng = (mode == GKAMUS_DIC_EN) ? TRUE : FALSE;
  g_idle_add ((GSourceFunc) set_sensitive_timer, window_main);
  g_idle_add ((GSourceFunc) restore_cursor, NULL);

  return FALSE;
}

/* 
 * FIXME: saya tidak tahu knp GtkRadioMenuItem @menu_tool_ei dan 
 * @menu_tool_ie sama2 di emit ketika merubah group radio button? 
 * walau group sudah diset, sementara pake var @eng utk tentukan
 * kamus yang aktif ;[
 */
void
on_menu_tool_ei_activate (GtkWidget *widget, 
                          gpointer   data)
{
  if (eng)
    return;

  if (modified)
    {
      gint ret;
      
      ret = create_dialog (GTK_MESSAGE_QUESTION, window_main, "Ganti Kamus",
                           "Simpan kamus sebelum mengganti model kamus?");
      if (ret == GTK_RESPONSE_OK)
        on_menu_save_activate (NULL, NULL);
    }
  set_sensitive (window_main, FALSE);
  change_cursor ();
  g_timeout_add (100, (GSourceFunc) change_dict, GINT_TO_POINTER (GKAMUS_DIC_EN));
  modified_notify (FALSE);
}

/* 
 * Menu Alat -> Kamus -> Indonesia - Inggris. 
 */
void
on_menu_tool_ie_activate (GtkWidget *widget, 
                          gpointer   data)
{
  if (!eng)
    return;

  if (modified)
    {
      gint ret;
      
      ret = create_dialog (GTK_MESSAGE_QUESTION, window_main, "Ganti Kamus",
                           "Simpan kamus sebelum mengganti model kamus?");
      if (ret == GTK_RESPONSE_OK)
        on_menu_save_activate (NULL, NULL);
    }
  set_sensitive (window_main, FALSE);
  change_cursor ();
  g_timeout_add (100, (GSourceFunc) change_dict, GINT_TO_POINTER (GKAMUS_DIC_ID));
  modified_notify (FALSE);
}

/*
 * Menu Alat -> Daftar abjad inggris. 
 */
void
on_menu_alpha_activate (GtkWidget *widget, 
                        gpointer   data)
{
  if (!window_alpha)
    window_alpha = create_window_alpha ();
  gtk_window_present ((GtkWindow *) window_alpha);
}

/* 
 * Menu Alat -> Tabel irregular verbs. 
 */
void
on_menu_verb_activate (GtkWidget *widget,
                       gpointer   data)
{
  if (window_verb)
    {
      gtk_window_present ((GtkWindow *) window_verb);
      return;
    }

  GIOChannel *read;
  gchar *file;
  const gchar verbs[] = "irregular-verbs";

#ifdef PACKAGE_DATA_DIR
  file = g_build_filename (PACKAGE_DATA_DIR, "gkamus", verbs, NULL);
#else
  file = g_build_filename ("share", "data", verbs, NULL);
#endif
  read = g_io_channel_new_file (file, "r", NULL);
  g_free (file);
  if (!read)
    {
      g_warning ("File irregular verbs tidak dapat dibuka.\n");
      create_dialog (GTK_MESSAGE_ERROR, window_main, "Error", "File irreguler "
                     "verbs tidak dapat dibuka.");
      return;
    }

  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *line, **split;

  window_verb = create_window_verb ();
  model = gtk_tree_view_get_model ((GtkTreeView *) treev_verb);
  while (g_io_channel_read_line (read, &line, NULL, NULL, NULL) != G_IO_STATUS_EOF)
    {
      split = g_strsplit_set (line, "\t\t\n", 3);
      if (g_strv_length (split) == 3)
        {
          gtk_list_store_append ((GtkListStore *) model, &iter);
          gtk_list_store_set ((GtkListStore *) model, &iter,
                               0, g_strstrip (split[0]),
                               1, g_strstrip (split[1]),
                               2, g_strstrip (split[2]),
                              -1);
        }
      g_free (line);
      g_strfreev (split);
    }
  g_io_channel_shutdown (read, TRUE, NULL);
  g_io_channel_unref (read);
  gtk_widget_show (window_verb);
}

#ifdef G_OS_UNIX
/* 
 * Menu Bantuan -> Buat shortcut -> Desktop. 
 */
void
on_menu_short_desktop_activate (GtkWidget *widget, 
                                gpointer   data)
{
  gint d = create_dialog (GTK_MESSAGE_QUESTION, window_main, "Buat Shortcut",
                          "gKamus akan membuat shortcut di Desktop, ini "
                          "mungkin tidak bekerja di sistem anda.");
  if (d != GTK_RESPONSE_OK)
    return;
  if (generate_shortcut (GKAMUS_SHORTCUT_DESKTOP) == -1)
    create_dialog (GTK_MESSAGE_WARNING, window_main, "Buat Shortcut",
                   "gKamus tidak dapat membuat shortcut di Desktop, "
                   "periksa permisi anda.");
}

/* 
 * Menu Bantuan -> Buat shortcut -> Menu.
 */
void
on_menu_short_menu_activate (GtkWidget *widget, 
                             gpointer   data)
{
  gint d = create_dialog (GTK_MESSAGE_QUESTION, window_main, "Buat Shortcut",
                          "gKamus akan membuat shortcut di Menu, ini "
                          "mungkin tidak bekerja di sistem anda.");
  if (d != GTK_RESPONSE_OK)
    return;
  if (generate_shortcut (GKAMUS_SHORTCUT_MENU) == -1)
    create_dialog (GTK_MESSAGE_WARNING, window_main, "Buat Shortcut",
                   "gKamus tidak dapat membuat shortcut di Menu, "
                   "periksa permisi anda.");
}
#endif

/*
 * Menu Bantuan -> Tentang. 
 */
void
on_menu_about_activate (GtkWidget *widget, 
                        gpointer   data)
{
  widget = create_dialog_about ();
  if (gtk_dialog_run ((GtkDialog *) widget) != GTK_RESPONSE_NONE)
    gtk_widget_destroy (widget);
}

/* 
 * Ketika list kata di pilih. 
 */
void 
on_treev_word_columns_changed (GtkWidget *widget, 
                               gpointer   data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_word);
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      Dictionary *dic;
      GtkTreePath *path;
      GtkTextBuffer *buffer;
      gint *id;

      buffer = gtk_text_view_get_buffer ((GtkTextView *) textv_definition);
      path = gtk_tree_model_get_path (model, &iter);
      id = gtk_tree_path_get_indices (path);
      dic = g_list_nth_data (dict, *id);

      gtk_text_buffer_set_text (buffer, dic->definition, -1);
      g_print ("selected: %s\nindices: %i\ntotal: %i\n\n", dic->word, 
               *id, g_list_length (dict));
      gtk_tree_path_free (path);
    }
}

/* 
 * Tombol paste, ambil data dari clipboard. 
 */
void 
on_button_paste_clicked (GtkWidget *widget, 
                         gpointer   data)
{
  on_menu_paste_activate (widget, data);
}

/* 
 * Cari kata di list.
 */
void
on_button_find_clicked (GtkWidget *widget, 
                        gpointer   data)
{
  const gchar *word = gtk_entry_get_text ((GtkEntry *) entry_search);
  if (!strlen (word))
    return;

  gchar *tmp0 = g_strdup (word);
  gchar *tmp = g_ascii_strdown (tmp0, -1);
  g_free (tmp0);
  tmp = g_strstrip (tmp);

  /* timer ini mungkin tidak benar, apalagi di win32 ;p */
  GTimer *timer = g_timer_new ();
  gint i = search_dict (tmp);

  if (i == -1)
    {
      g_print ("search: %s [not found]\n", word);
      i = create_dialog (GTK_MESSAGE_QUESTION, window_main, "Pencarian",
                         "Kata yang dicari tidak ditemukan, tambah kata ini?");
      if (i == GTK_RESPONSE_OK)
        {
          window_modify = create_window_modify (GKAMUS_MODIFY_ADD);
          gtk_entry_set_text ((GtkEntry *) entry_mod, tmp);
          gtk_widget_show (window_modify);
        }
    }
  else
    {
      g_timer_stop (timer);
      gdouble t = g_timer_elapsed (timer, NULL);
      g_print ("search time: %f second(s).\n", t);
      select_treev_word (i);
    }
  g_timer_destroy (timer);
  g_free (tmp);
}

gboolean
on_window_alpha_delete_event (GtkWidget *widget, 
                              GdkEvent  *event,
                              gpointer   data)
{
  gtk_widget_hide (window_alpha);
  return TRUE;
}

void
on_window_verb_destroy (GtkWidget *widget,
                        gpointer   data)
{
  gtk_widget_destroy (window_verb);
  window_verb = NULL;
}


/*
 *  function.c, gKamus (http://gkamus.sourceforge.net)
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
# include <config.h>
#endif

#include <gtk/gtk.h>

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "gui.h"

static gchar** divide_line (const gchar*, const gchar*);
static gboolean check_dict (gpointer);

/* 
 * Simpan file kamus.
 */
gsize
write_dict (const gchar *file)
{
  g_return_val_if_fail (file != NULL, 0);

  GIOChannel *write;

  if (!(write = g_io_channel_new_file (file, "w", NULL)))
    return 0;

  Dictionary *dic;
  GList *list = dict;
  gsize ret, bytes;
  gchar *tmp;

  static const gchar head[] =
  (
    "[gKamus File Format v1.0]\n[Encoding ANSI]\n[File ini adalah \"public"
    " domain\", tidak ada hak cipta]\n[Ditulis oleh: Firmansyah, Ardhan"
    " Madras]\n[Website http://gkamus.sourceforge.net]\n\n"
  );

  g_io_channel_write_chars (write, head, -1, &bytes, NULL);
  ret = bytes;
  while (list)
    {
      dic = list->data;
      tmp = g_strconcat (dic->word, "\t", dic->definition, "\n", NULL);
      g_io_channel_write_chars (write, tmp, -1, &bytes, NULL);
      ret += bytes;
      g_free (tmp);
      list = list->next;
    }
  g_io_channel_shutdown (write, TRUE, NULL);
  g_io_channel_unref (write);
  
  return ret;
}

/* 
 * Cek validitas format, simpel saja ;p 
 */
static gboolean
check_dict (gpointer read)
{
  g_return_val_if_fail (read != NULL, FALSE);

  GIOStatus status;
  gchar *line;
  gsize len;
  gboolean ret;

  status = g_io_channel_read_line ((GIOChannel *) read, &line, &len, NULL, NULL);
  if (status == G_IO_STATUS_ERROR)
    {
      if (len)
        g_free (line);
      return FALSE;
    }
  ret = !strcmp (line, "[gKamus File Format v1.0]\n");
  g_free (line);

  return ret;
}

/*
 * Bagi baris (line) kamus menjadi kata dan definisinya.
 * free dengan g_strfreev().
 */ 
static gchar**
divide_line (const gchar *line, 
             const gchar *delimeter)
{
  g_return_val_if_fail (line != NULL, NULL);
  g_return_val_if_fail (delimeter != NULL, NULL);

  /* cek '[' dan line kosong */
  if (*line == '[' || *line == '\n')
    return NULL;
  gchar **split = g_strsplit_set (line, delimeter, -1);
  if (g_strv_length (split) != 2)
    {
      g_strfreev (split);
      split = NULL;
    }

  return split;
}

/*
 * Cari kata di double linked list GList (linear search).
 */
gint
search_dict (const gchar *word)
{
  g_return_val_if_fail (word != NULL, -1);

  GList *list = dict;
  Dictionary *dic;
  register gint i;

  for (i = 0; list; list = list->next, i++)
    {
      dic = list->data;
      if (!strcmp (word, dic->word))
        return i;
    }

  return -1;
}

/*
 * Fungsi perbandingan list untuk sortir.
 */
gint
list_compare (const Dictionary *a, 
              const Dictionary *b)
{
  return strcmp (a->word, b->word);
}

void
free_elements (Dictionary *dic, 
               gpointer    data)
{
  g_return_if_fail (dic != NULL);
  g_free (dic->word);
  g_free (dic->definition);
  g_free (dic);
}

/*
 * Load kamus ke memori (GList).
 */
GkamusDicStatus
load_dict (const gchar *file)
{
  g_return_val_if_fail (file != NULL, GKAMUS_DIC_STATUS_NOT_FOUND);

  GIOChannel *read;
  gchar *temp;
  gboolean test;

#ifdef PACKAGE_DATA_DIR
  temp = g_build_filename (PACKAGE_DATA_DIR, "gkamus", file, NULL);
#else
  temp = g_build_filename (g_get_current_dir (), "share", "data", file, NULL);
#endif

  test = g_file_test (temp, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
  if (!test)
    {
      g_free (temp);
      return GKAMUS_DIC_STATUS_NOT_FOUND;
    }
  read = g_io_channel_new_file (temp, "r", NULL);
  if (!read)
    {
      g_free (temp);
      return GKAMUS_DIC_STATUS_ERROR;
    }
  if (!check_dict (read)) /* format valid ? */
    {
      g_free (temp);
      g_io_channel_shutdown (read, TRUE, NULL);
      g_io_channel_unref (read);
      return GKAMUS_DIC_STATUS_INVALID;
    }

  gchar **split;
  gchar *line, *word, *definition;
  gint i = 0;

  /* free list dan element */
  if (dict != NULL)
    {
      g_list_foreach (dict, (GFunc) free_elements, NULL);
      g_list_free (dict);
      dict = NULL;
    }
  while (g_io_channel_read_line (read, &line, NULL, NULL, NULL) != G_IO_STATUS_EOF)
    {
      if (i > MAX_ENTRY) /* periksa entri maksimal */
        {
          g_io_channel_shutdown (read, TRUE, NULL);
          g_io_channel_unref (read);
          g_free (line);
          g_free (temp);
          return GKAMUS_DIC_STATUS_MAX_ENTRY;
        }
      if ((split = divide_line (line, "\t")))
        {
          Dictionary *dic = g_new (Dictionary, 1);
          word = g_ascii_strdown (g_strstrip (split[0]), -1);
          definition = g_strstrip (split[1]);
          dic->word = g_strdup (word);
          dic->definition = g_strdup (definition);
          g_free (word);

          /* g_list_insert() atau g_list_append() tentu akan lama untuk
           * data yang banyak, disini kita masukkan @dic pada head (awal)
           * list sort list sesuai abjad
           */
          dict = g_list_prepend (dict, dic);
          i++;
       }
      g_free (line);
      g_strfreev (split);
    }
  /* reverse list dengan sort, kita tidak perlu g_list_reverse() */
  dict = g_list_sort (dict, (GCompareFunc) list_compare);

  /* set @dict_file */
  if (dict_file)
    g_free (dict_file);
  dict_file = g_strdup (temp);
  g_free (temp);

  g_io_channel_shutdown (read, TRUE, NULL);
  g_io_channel_unref (read);

  return GKAMUS_DIC_STATUS_OK;
}

/*
 * Masukkan kata ke treeview.
 */
void
set_treev_word (void)
{
  GList *ptr;
  Dictionary *d;
  GtkTreeModel *model;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model ((GtkTreeView *) treev_word);
  gtk_list_store_clear ((GtkListStore *) model);
  ptr = dict;
  while (ptr)
    {
      d = ptr->data;
      gtk_list_store_append ((GtkListStore *) model, &iter);
      gtk_list_store_set ((GtkListStore *) model, &iter, COLUMN_WORD, d->word, -1);
      ptr = g_list_next (ptr);
    }
}

void
show_dicstatus (GkamusDicStatus status)
{
  static const gchar msg[] = "Silahkan perbaiki kesalahan pada kamus atau "
                             "lakukan instalasi\nulang bila perlu.";
  create_dialog (GTK_MESSAGE_ERROR, window_main, dic_errmsg[status], msg);
}

/*
 * Shortcut desktop di UNIX. 
 */
#ifdef G_OS_UNIX
gint
generate_shortcut (GkamusShortcutMode mode)
{
  g_return_val_if_fail (mode >= GKAMUS_SHORTCUT_DESKTOP &&
                        mode <= GKAMUS_SHORTCUT_MENU, -1);

  GIOChannel *write;
  gchar *tmp;

  if (mode == GKAMUS_SHORTCUT_DESKTOP)
    tmp = g_build_filename (g_get_home_dir (), "Desktop", "gkamus.desktop", NULL);
  else
    tmp = g_build_filename ("/usr/local/share/applications", "gkamus.desktop", NULL);

  write = g_io_channel_new_file (tmp, "w", NULL);
  g_free (tmp);
  if (!write)
    return -1;

  GKeyFile *key = g_key_file_new ();
  GIOStatus stat;
  static const gchar group[] = "Desktop Entry";

  g_key_file_set_string (key, group, "Version", "1.0");
  g_key_file_set_string (key, group, "Name", PROGRAM_NAME);
  g_key_file_set_string (key, group, "Comment", PROGRAM_DESC);
  
  /* jika dijalankan dengan relative path, seperti
   * ./gkamus atau apps/gkamus, kita perlu cari absolute path
   * dengan kombinasi lokasi working direktori + argv[0]
   */
  if (*prg_name != '/')
    tmp = g_build_filename (g_get_current_dir (), prg_name, NULL);
  else
    tmp = g_strdup (prg_name);
  g_key_file_set_string (key, group, "Exec", tmp);
  g_free (tmp);

#ifdef PACKAGE_PIXMAPS_DIR
  tmp = g_build_filename (PACKAGE_PIXMAPS_DIR, "gkamus.png", NULL);
#else
  tmp = g_build_filename (g_get_current_dir (), "share", "pixmaps", "gkamus.png", NULL);
#endif

  g_key_file_set_string (key, group, "Icon", tmp);
  g_key_file_set_boolean (key, group, "Terminal", FALSE);
  g_key_file_set_string (key, group, "Type", "Application");
  g_key_file_set_string (key, group, "Categories", "Application;Utility");
  g_key_file_set_boolean (key, group, "StartupNotify", TRUE);
  g_key_file_set_boolean (key, group, "Terminal", FALSE);

  g_free (tmp);
  tmp = g_key_file_to_data (key, NULL, NULL);
  tmp = g_strstrip (tmp);
  g_key_file_free (key);
  stat = g_io_channel_write_chars (write, tmp, -1, NULL, NULL);
  g_free (tmp);
  g_io_channel_shutdown (write, TRUE, NULL);
  g_io_channel_unref (write);
  
  return (stat != G_IO_STATUS_ERROR) ? 0 : -1;
}
#endif

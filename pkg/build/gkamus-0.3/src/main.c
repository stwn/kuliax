/*
 *  main.c, gKamus (http://gkamus.sourceforge.net)
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

/*
 *  18-Feb-2008, 13.32 WIB, Dream Theater "Perfect Stranger".
 */

#include <gtk/gtk.h>

#include "main.h"
#include "gui.h"
#include "callback.h"
#include "function.h"

GList *dict = NULL; /* struktur data kamus (double linked list) */
gchar *dict_file = NULL; /* path absolute kamus yang sedang aktif */
const gchar const * dict_en	= "gkamus-en.dict";	/* nama file kamus inggris */
const gchar const * dict_id	= "gkamus-id.dict";	/* nama file kamus indonesia */
gchar *prg_name = NULL; /* string untuk argv[1] */

const gchar * const dic_errmsg[] =
{
  "File kamus tidak dapat dibuka",
  "File kamus tidak ditemukan",
  "Format file salah atau tidak diketahui",
  "Jumlah entri kamus melewati batas maksimal",
  "File kamus ok"
};


gint
main (gint   argc, 
      gchar *argv[])
{
  gtk_init (&argc, &argv);

  /* set prg_name ke argv[0] */
  prg_name = argv[0];

	window_main = create_window_main (); /* buat window utama gkamus */

	/* load file kamus */
	gint status = load_dict (dict_en);
  if (status != GKAMUS_DIC_STATUS_OK)
    {
      show_dicstatus (status);
      return -1;
    }

  gtk_widget_show_all (window_main);
  set_treev_word ();
	select_treev_word (0);

	gtk_main ();
	
	gtk_widget_destroy (window_main);
	if (window_alpha)
    gtk_widget_destroy (window_alpha);
  if (window_verb)
    gtk_widget_destroy (window_verb);
	g_list_foreach (dict, (GFunc) free_elements, NULL);
	g_list_free (dict);

	return 0;
}

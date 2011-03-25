/*
 * Gyatzee: Gnomified Yahtzee game.
 * (C) 1998 the Free Software Foundation
 *
 * File:   clist.c
 *
 * Author: Scott Heavner
 *
 *   Scoring is done using a GtkTreeView and handled in this file.
 *
 *   Variables are exported in gyahtzee.h
 *
 *   This file is largely based upon GTT code by Eckehard Berns.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <config.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <stdio.h>

#include "yahtzee.h"
#include "gyahtzee.h"

static gchar *row_tooltips[MAX_FIELDS];

void
update_score_cell (GtkWidget * treeview, gint row, gint col, int val)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  char *buf;

  g_assert (treeview != NULL);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  gtk_tree_model_iter_nth_child (model, &iter, NULL, row);
  if (val < 0)
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, "", -1);
  else {
    buf = g_strdup_printf ("%i", val);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, col, buf, -1);
    g_free (buf);
  }
}

static void
set_label_bold (GtkLabel * label, gboolean make_bold)
{
  PangoAttrList *attrlist;
  PangoAttribute *attr;

  g_assert (label != NULL);

  attrlist = gtk_label_get_attributes (label);
  if (!attrlist)
    attrlist = pango_attr_list_new ();

  if (make_bold) {
    attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
    attr->start_index = 0;
    attr->end_index = -1;
    pango_attr_list_change (attrlist, attr);
  } else {
    attr = pango_attr_weight_new (PANGO_WEIGHT_NORMAL);
    attr->start_index = 0;
    attr->end_index = -1;
    pango_attr_list_change (attrlist, attr);
  }
  gtk_label_set_attributes (label, attrlist);
}

/* Shows the active player by make the player name bold in the TreeView. */
void
ShowoffPlayer (GtkWidget * treeview, int player, int so)
{
  GtkTreeViewColumn *col;
  GtkWidget *label;
  GList *collist;

  g_return_if_fail (treeview != NULL);

  if (player < 0 || player >= MAX_NUMBER_OF_PLAYERS)
    return;

  collist = gtk_tree_view_get_columns (GTK_TREE_VIEW (treeview));
  col = GTK_TREE_VIEW_COLUMN (g_list_nth_data (collist, player + 1));
  g_list_free (collist);

  label = gtk_tree_view_column_get_widget (col);
  if (!label)
    return;

  g_assert (GTK_IS_LABEL (label));

  set_label_bold (GTK_LABEL (label), so);
}

static gint gtk_tree_path_to_row (GtkTreePath *path)
{
  char *path_str = gtk_tree_path_to_string (path);
  gint  row;
  if (sscanf (path_str, "%i", &row) != 1) {
    g_warning ("%s: could not convert '%s' to integer\n",
	       G_STRFUNC, path_str);
    g_free (path_str);
    return -1;
  }
  g_free (path_str);
  return row;
}

/* Convert the row of a tree into the score row */

static gint score_row(GtkTreePath *path)
{
    gint row = gtk_tree_path_to_row (path);
    if (row == R_UTOTAL || row == R_BONUS || row == R_BLANK1 ||
        row == R_GTOTAL || row == R_LTOTAL)
        return -1;

    /* Adjust for Upper Total / Bonus entries */
    if (row >= NUM_UPPER)
        row -= 3;

    if (row < 0 || row >= NUM_FIELDS)
        return -1;

    return row;
}

static void
row_activated_cb (GtkTreeView * treeview, GtkTreePath * path,
		  GtkTreeViewColumn * column, gpointer user_data)
{
  int row = score_row (path);

  if (players[CurrentPlayer].comp)
    return;

  if (row >= 0) {
    if (row < NUM_FIELDS && !players[CurrentPlayer].finished) {
      if (play_score (CurrentPlayer, row) == SLOT_USED) {
        say (_("Already used! " "Where do you want to put that?"));
      } else {
        UndoScoreElement *elem = RedoHead();
        if (elem && elem->player == CurrentPlayer) {
          RedoPlayer();
        } else {
          NextPlayer ();
        }
      }
    }
  }
  update_undo_sensitivity();
}

static gboolean
activate_selected_row_idle_cb (gpointer data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (data);
  GtkTreeViewColumn *column;
  GtkTreePath *path;

  path = NULL;
  gtk_tree_view_get_cursor (tree, &path, &column);
  if (path) {
    if (!column)
      column = gtk_tree_view_get_column (tree, 0);
    gtk_tree_view_row_activated (tree, path, column);
  }

  /* Quoted from docs: "The returned GtkTreePath must be freed 
   * with gtk_tree_path_free() when you are done with it." */
  gtk_tree_path_free (path);

  return FALSE;
}

/* Returns: FALSE to let the GtkTreeView focus the selected row */
static gboolean
tree_button_press_cb (GtkWidget * widget, GdkEventButton * event,
		      gpointer data)
{
  GtkTreeView *tree = GTK_TREE_VIEW (data);

  g_assert (widget != NULL);
  g_assert (event != NULL);

  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  g_idle_add_full (G_PRIORITY_HIGH, activate_selected_row_idle_cb,
		   (gpointer) tree, NULL);

  return FALSE;
}

/* Returns: TRUE to let GtkTreeView know it can show a tooltip */
static gboolean
tree_query_tooltip_cb (GtkWidget * widget, gint x, gint y,
                       gboolean keyboard_mode, GtkTooltip *tooltip, gpointer data)
{
    GtkTreeModel *model_ptr = NULL;
    GtkTreePath  *path_ptr  = NULL;
    GtkTreeIter  *iter_ptr  = NULL;
    gint rval = FALSE;

    if (gtk_tree_view_get_tooltip_context ( GTK_TREE_VIEW (widget), &x, &y,
                keyboard_mode, &model_ptr, &path_ptr, iter_ptr)) {
        if (path_ptr) {
            gint row = score_row(path_ptr);
            if (row >= 0) {
                gtk_tooltip_set_text (tooltip, row_tooltips[row]);
                rval = TRUE;
            }
        }
    }

    return rval;
}

GtkWidget *
create_score_list (void)
{
  GtkWidget *tree;
  GtkListStore *store;

  store = gtk_list_store_new (MAX_NUMBER_OF_PLAYERS + 3,
			      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (tree), FALSE);
  gtk_widget_set_has_tooltip (GTK_WIDGET (tree), TRUE);

  g_object_unref (store);

  g_signal_connect (G_OBJECT (tree), "row-activated",
		    G_CALLBACK (row_activated_cb), NULL);
  g_signal_connect (G_OBJECT (tree), "button-press-event",
		    G_CALLBACK (tree_button_press_cb), (gpointer) tree);
  g_signal_connect (G_OBJECT (tree), "query-tooltip",
            G_CALLBACK (tree_query_tooltip_cb), (gpointer) tree);

  return tree;
}

static void
add_columns (GtkTreeView * tree)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *label;
  GValue *prop_value;
  int i;

  /* Create columns */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
						     "text", 0,
						     "weight", LAST_COL,
						     NULL);
  g_object_set (renderer, "weight-set", TRUE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  for (i = 0; i < MAX_NUMBER_OF_PLAYERS; i++) {
    renderer = gtk_cell_renderer_text_new ();
    prop_value = g_new0 (GValue, 1);
    g_value_init (prop_value, G_TYPE_FLOAT);
    g_value_set_float (prop_value, 1.0);
    g_object_set_property (G_OBJECT (renderer), "xalign", prop_value);
    g_object_set (renderer, "weight-set", TRUE, NULL);
    g_value_unset (prop_value);
    g_free (prop_value);

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer,
					 "text", i + 1,
					 "weight", LAST_COL, NULL);
    gtk_tree_view_column_set_min_width (column, 95);
    gtk_tree_view_column_set_alignment (column, 1.0);
    label = gtk_label_new (players[i].name);
    gtk_tree_view_column_set_widget (column, label);
    gtk_widget_show (label);
    gtk_tree_view_column_set_visible (column, FALSE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  }
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer, "text",
						     MAX_NUMBER_OF_PLAYERS
						     + 1, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
}

void
score_list_set_column_title (GtkWidget * treeview, int column,
			     const char *str)
{
  GtkTreeViewColumn *col;
  GtkWidget *label;

  g_assert (treeview != NULL);

  col = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), column);
  label = gtk_tree_view_column_get_widget (col);
  if (!label)
    return;

  gtk_label_set_text (GTK_LABEL (label), str);
}

static void
initialize_column_titles (GtkTreeView * treeview)
{
  GtkTreeViewColumn *col;
  GList *collist, *node;
  GtkWidget *label;
  int i;

  collist = gtk_tree_view_get_columns (treeview);
  i = 0;
  for (node = collist; node != NULL; node = g_list_next (node)) {
    col = GTK_TREE_VIEW_COLUMN (node->data);
    label = gtk_tree_view_column_get_widget (col);
    if (!label)
      continue;

    gtk_tree_view_column_set_visible (col, i < NumberOfPlayers);
    gtk_label_set_text (GTK_LABEL (label), players[i].name);
    i++;
  }
  g_list_free (collist);
}

void
setup_score_list (GtkWidget * treeview)
{
  GtkTreeModel *model;
  GtkListStore *store;
  GtkTreeIter iter;
  GList *columns;
  int i;

  g_assert (treeview != NULL);

  columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (treeview));
  if (!columns) {
    add_columns (GTK_TREE_VIEW (treeview));
  } else {
    initialize_column_titles (GTK_TREE_VIEW (treeview));
  }

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  store = GTK_LIST_STORE (model);
  gtk_list_store_clear (store);

  for (i = 0; i < NUM_UPPER; i++) {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, _(FieldLabels[i]),
			LAST_COL, PANGO_WEIGHT_NORMAL, -1);
  }

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, _(FieldLabels[F_BONUS]),
		      LAST_COL, PANGO_WEIGHT_BOLD, -1);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, _(FieldLabels[F_UPPERT]),
		      LAST_COL, PANGO_WEIGHT_BOLD, -1);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, "", -1);

  for (i = 0; i < NUM_LOWER; i++) {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			0, _(FieldLabels[i + NUM_UPPER]),
			LAST_COL, PANGO_WEIGHT_NORMAL, -1);
  }

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, _(FieldLabels[F_LOWERT]),
		      LAST_COL, PANGO_WEIGHT_BOLD, -1);
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, _(FieldLabels[F_GRANDT]),
		      LAST_COL, PANGO_WEIGHT_BOLD, -1);
}

void
update_score_tooltips(void)
{
    gint ii;

    for (ii = 0; ii < NUM_FIELDS; ii++) {
        gint score = player_field_score (CurrentPlayer, ii);
        if (!row_tooltips[ii]) row_tooltips[ii] = g_new0(gchar, 100);


        if (score >= 0)
            sprintf(row_tooltips[ii], _("Score: %d"), score);
        else
            sprintf(row_tooltips[ii], _("Field used"));
    }
}

/* Arrgh - lets all use the same tabs under emacs: 
Local Variables:
tab-width: 8
c-basic-offset: 8
indent-tabs-mode: nil
End:
*/

#include "glib.h"
#include <gtk/gtk.h>

#ifndef UI_H
#define UI_H


extern gulong idle_handler ,auto_update_handler;

void get_song_name(char* filename);

void get_song_length(double total_length);

void get_song_pos(int pos);

static void next_button_callback (GtkWindow *widget, gpointer   data);

gboolean update_seekbar(gpointer position);

int
ui_main (int    argc,
      char **argv);

#endif // UI_H

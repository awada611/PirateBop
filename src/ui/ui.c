#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtkcssprovider.h"
#include "gtk/gtkshortcut.h"
#include "../../include/player.h"
static GtkWidget *song_label = NULL;
static GtkWidget *album_title_label = NULL;
static GtkWidget *cover_pic_label = NULL;
static GtkWidget *time_label = NULL;
static GtkWidget *seek_bar = NULL;
double length;
gulong handler_id, handle_id;
_Atomic(bool) is_dragging;
static char *css_buffer = NULL;

//to check if dir is empty and prevent crashes
bool empty_dir_behaviour(){
    if(get_total()==0) return false;
    return true;
}


//unified songinfo text update
static
void update_song_info(){
    char* text = g_strdup_printf("%s- %s - %s", song_info->track_number, song_info->title, song_info->artist);
    if(song_label) gtk_label_set_text(GTK_LABEL(song_label), text);
    if(album_title_label) gtk_label_set_text(GTK_LABEL(album_title_label), song_info->album);
    if(cover_pic_label && song_info->cover_size>0 && song_info->cover_ptr != NULL){
        GBytes* bytes = g_bytes_new_static(song_info->cover_ptr, song_info->cover_size);
        GdkPixbuf *pixbuff = gdk_pixbuf_new_from_stream(g_memory_input_stream_new_from_bytes(bytes), NULL, NULL);
        if(pixbuff){
            gtk_image_set_from_pixbuf(GTK_IMAGE(cover_pic_label), pixbuff);
            g_object_unref(pixbuff);
        }
    } else {
        gtk_image_clear(GTK_IMAGE(cover_pic_label));
        gtk_image_set_from_resource(GTK_IMAGE(cover_pic_label), "/org/piratebop/pirate.png");
    }
    g_free(text);
}


//song end
gboolean end_song(){
    bool end_song = song_is_ended();
    if(end_song) {
        next();
        update_song_info();
    }
    return G_SOURCE_CONTINUE;
}

gboolean update_total_length(gpointer position) {
    int song_length = get_length();
    if(song_length<0) {
        char* over_song = g_strdup_printf("00:00");
        gtk_label_set_text(GTK_LABEL(time_label), over_song);
        return 0;
    }
    int minutes = (int)song_length / 60;
    int seconds = (int)song_length % 60;
    char* disp_length = g_strdup_printf("%02d:%02d", minutes, seconds);
    if(time_label) gtk_label_set_text(GTK_LABEL(time_label), disp_length);
    return G_SOURCE_CONTINUE;
}

static void
play_button_callback (GtkWidget *widget,
                      gpointer   data)
{
    if(!empty_dir_behaviour()) return;
    play();
    update_song_info();
}

static void
pause_button_callback(GtkWindow *widget,
                      gpointer   data)
{
    if(!empty_dir_behaviour()) return;
    pause();
}

static void
stop_button_callback (GtkWindow *widget,
                      gpointer   data)
{
    if(!empty_dir_behaviour()) return;
    stop();
}

static void
next_button_callback (GtkWindow *widget,
                      gpointer   data)
{
    if(!empty_dir_behaviour()) return;
    next();
    update_song_info();

}

static void
prev_button_callback (GtkWindow *widget,
                      gpointer   data)
{
   if(!empty_dir_behaviour()) return;
   prev();
   update_song_info();

}

static void
shuffle_callback (GtkWindow *widget,
                      gpointer   data)
{
    if(!empty_dir_behaviour()) return;
    int zabre = shuffle_toggle();
    printf("%d\n", zabre);

}

/////////////////////////////SeekBar///////////////////
static void
seek_bar_callback (GtkWindow *widget,
                      gpointer   data)
{
    atomic_store(&is_dragging, true) ;
    int new_position = gtk_range_get_value(GTK_RANGE(seek_bar));
    seek(new_position);
    atomic_store(&is_dragging, false);
}

gboolean update_seekbar(gpointer position) {
    if (!seek_bar) return G_SOURCE_CONTINUE;

    if (atomic_load(&is_dragging) == false) {
        int new_position = get_position();
        g_signal_handler_block(seek_bar, handler_id);
        gtk_range_set_value(GTK_RANGE(seek_bar), new_position);
        g_signal_handler_unblock(seek_bar, handler_id);
    }
    return G_SOURCE_CONTINUE;
}

///////////////////////////////////////////////////////


static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *play_button, *pause_button, *stop_button, *next_button, *prev_button, *shuffle;
  GtkWidget *bigBox, *box, *label;


  GtkCssProvider *styles = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(styles, "/org/piratebop/styles.css");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
        GTK_STYLE_PROVIDER(styles), GTK_STYLE_PROVIDER_PRIORITY_USER);


  //loads song as soon as window opens
  song_label = gtk_label_new("");
  album_title_label = gtk_label_new("");
  cover_pic_label = gtk_image_new();
  gtk_widget_set_size_request(cover_pic_label, 200, 200);
  time_label = gtk_label_new("");

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "PirateBop");
  gtk_window_set_default_size (GTK_WINDOW (window), 1366, 768);
  gtk_window_set_icon_name(GTK_WINDOW(window), "pirate");


  bigBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
  GtkWidget *image_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 100);
  gtk_widget_set_halign(image_container, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image_container, GTK_ALIGN_CENTER);


  gtk_widget_set_halign(song_label, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);

  gtk_window_set_child (GTK_WINDOW (window), bigBox);



  // Button creation:
  play_button = gtk_button_new_with_label("▶");
  pause_button = gtk_button_new_with_label("⏸");
  stop_button = gtk_button_new_with_label("⏹");
  next_button = gtk_button_new_with_label ("⏭");
  prev_button = gtk_button_new_with_label ("⏮");
  shuffle = gtk_button_new_with_label ("⇄");

  seek_bar = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  g_timeout_add(500, update_seekbar, NULL);


  void update_song_info();

  g_signal_connect (play_button, "clicked", G_CALLBACK (play_button_callback), NULL);
  g_signal_connect(pause_button, "clicked", G_CALLBACK (pause_button_callback), NULL);
  g_signal_connect (stop_button, "clicked", G_CALLBACK (stop_button_callback), NULL);
  g_signal_connect (next_button, "clicked", G_CALLBACK (next_button_callback), NULL);
  g_signal_connect (prev_button, "clicked", G_CALLBACK (prev_button_callback), NULL);
  g_signal_connect (shuffle, "clicked", G_CALLBACK (shuffle_callback), NULL);
  handler_id = g_signal_connect(seek_bar, "value-changed", G_CALLBACK(seek_bar_callback), NULL);


  gtk_box_append(GTK_BOX (bigBox), label);
  gtk_box_append(GTK_BOX(bigBox), image_container);
  gtk_box_append(GTK_BOX(image_container), cover_pic_label);
  gtk_box_append (GTK_BOX (label), song_label);
  gtk_box_append (GTK_BOX (bigBox), album_title_label);
  gtk_box_append (GTK_BOX (label), time_label);
  gtk_box_append (GTK_BOX (bigBox), box);
  gtk_box_append (GTK_BOX (bigBox), seek_bar);
  gtk_box_append (GTK_BOX (box), play_button);
  gtk_box_append (GTK_BOX (box), pause_button);
  gtk_box_append(GTK_BOX (box), stop_button);
  gtk_box_append(GTK_BOX (box), next_button);
  gtk_box_append(GTK_BOX (box), prev_button);
  gtk_box_append(GTK_BOX (box), shuffle);

  gtk_window_present (GTK_WINDOW (window));
}

int
ui_main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  g_timeout_add(1000, update_total_length, NULL);
  g_timeout_add(10, end_song, NULL);

  app = gtk_application_new ("org.example.PirateBop", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (app);

  return status;
}

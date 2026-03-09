#ifndef PLAYLIST_H
#define PLAYLIST_H


typedef struct Song{
    char* filename;
    double total_length;
}Song;

typedef struct PlaylistSong{
    Song* song;
    int index;
}PlaylistSong;


void init_array_of_songs(PlaylistSong** playlist, int total);

void expand_array_of_songs(PlaylistSong** playlist, int new_total, int last_index);

Song* create_song_entry(char* filename);

void build_playlist_from_dir();
void playlist_cleanup();

PlaylistSong* get_song(int index);

int get_shuffled_song();

int get_total();

#endif

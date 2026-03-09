#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <stdatomic.h>

#include "miniaudio.h"
#include "playlist.h"
#include "meta_reader.h"


typedef struct Player{
  PlaylistSong* playlist;
  int* shuffled_array;
  int shuffle_count;
  int current_index;
  int song_total;
  bool is_initialized;
  bool is_playing;    // flag to declare player playing song
  bool playback_stop; // flag set when song is stopped
  bool playback_end;  // flag set when song ends naturally (reaches end frame)
  int position;
  ma_uint64 frame;
  ma_uint64 total_frames;
  double song_length;
  _Atomic(bool) seek_pending;
  _Atomic(bool) shuffle;
}Player;

extern MetaInfo* song_info;

int get_position();
bool song_is_ended();
int get_length();

int shuffle_toggle();

void init_playlist();

void init_shuffled_playlist();

void init_player_state();

void update_player_state();

int init_playback();
int update_playback();

int play();

int pause();

int stop();

int next();

int prev();

void seek(int new_position);

char* get_songname();

void create_shuffled_array();
void shuffle();

void player_cleanup();

#endif // PLAYER_H

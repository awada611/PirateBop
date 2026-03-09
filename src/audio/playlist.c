#include "../../include/playlist.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static PlaylistSong* playlist = NULL;
static int total = 512;
static int song_count = 0;
static int* shuffled_array = NULL;

void init_array_of_songs(PlaylistSong** playlist, int total){
   *playlist = malloc(total * sizeof(PlaylistSong)) ;

   if(*playlist==NULL){
       printf("failed to initialize playlist!!");
       return;
   }
   for (int i=0;i<total;i++){
       (*playlist)[i].song = NULL;
       (*playlist)[i].index = 0;
   }

}

void expand_array_of_songs(PlaylistSong** playlist, int new_total, int last_index){
    PlaylistSong* temp = realloc(*playlist, new_total*sizeof(PlaylistSong));
    if(!temp) {
        printf("failed to expand playlist!");
    };
    *playlist =temp;

    if (*playlist == NULL){
        printf("failed to expand playlist");
        return;
    }
    for(int i=last_index+1; i < new_total; i++){
        (*playlist)[i].song=NULL;
        (*playlist)[i].index=0;
    }
}

Song* create_song_entry(char* filename){
    Song* song = malloc(sizeof(Song));
    song->filename = filename;
    return song;
}

void build_playlist_from_dir(){
    struct dirent *entry;
    DIR* dir;
    char* filename;
    int old_count=0;
    Song *new_song;


    init_array_of_songs(&playlist, total);

    dir = opendir(".");
    if(dir == NULL){
        perror("opendir");
    }

    for(int i=0;((entry=readdir(dir))!= NULL); i++ ){

        if(strstr(entry->d_name,".mp3" ) || strstr(entry->d_name,".flac" ) || strstr(entry->d_name,".wav" )){
            if(old_count >= total){
                total *= 2;
                expand_array_of_songs(&playlist, (total), old_count);
            }
           filename = strdup(entry->d_name);
           new_song = create_song_entry(filename);
           playlist[old_count].song = new_song;
           playlist[old_count].index = old_count;
           old_count++;
           //printf("%d. %s\n", old_count, playlist[old_count].song->filename);
        }
    }
    song_count=old_count;
    closedir(dir);
}

void playlist_cleanup(){
    for(int i = 0; i<song_count; i++){
        if(playlist[i].song){
            if(playlist[i].song->filename){
                free(playlist[i].song->filename);
            }
            free(playlist[i].song);
        }
    }
    free(playlist);
    playlist = NULL;
    song_count = 0;
    total = 512;
}

PlaylistSong* get_song(int index){
    if(index<0 || index>= total){
        return NULL;
    }
    return &playlist[index];
}

/*
int get_shuffled_song() {
    unsigned int r;
    arc4random_buf(&r, sizeof(r));
    return r % song_count;
    }
void create_shuffled_array(){
    shuffled_array = malloc(song_count*sizeof(int));
    if(!shuffled_array){
        printf("dammit!!");
        return;
    }
    for(int i = 0; i < song_count; i++){
        shuffled_array[i]=i;
    }
}

void swap (int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void shuffle(){
    create_shuffled_array();

    srand ( time(NULL) );

    for (int i = song_count-1; i > 0; i--)
        {
            // Pick a random index from 0 to i
            int j = rand() % (i+1);

            // Swap arr[i] with the element at random index
            swap(&shuffled_array[i], &shuffled_array[j]);
        }
}

*/
int get_total(){
    return song_count;
}
/*
int main() {
    build_playlist_from_dir();

    int total = get_total();
    printf("Found %d songs:\n", total);
    printf("----------------\n");

    for (int i = 0; i < total; i++) {
        PlaylistSong *song = get_song(i);
        if (song && song->song && song->song->filename) {
            printf("[%d] %s\n", song->index, song->song->filename);
        }
    }

    playlist_cleanup();
    return 0;
}
*/

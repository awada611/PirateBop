#include <stdatomic.h>
#include <unistd.h>
#define MINIAUDIO_IMPLEMENTATION

#include "../../include/player.h"
#include <stdio.h>

//global static variables shared between all player functions
static ma_decoder decoder;
static ma_device device;
static ma_device_config deviceConfig;
static Player *player = NULL;
MetaInfo* song_info = NULL;



void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }

    ma_result result = ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);
    player->frame += frameCount;

    player->position = 100*(player->frame)/(player->total_frames);

    player->song_length = (double)(player->total_frames - player->frame) / pDecoder->outputSampleRate;


    if(atomic_load(&player->seek_pending)==true){
        ma_decoder_seek_to_pcm_frame(&decoder, player->frame);
        atomic_store(&player->seek_pending, false);
    }

    if(result == MA_AT_END){
        player->frame=0;
        player->position=0;
        player->playback_stop = true;
        player->playback_end = true;
    }

    (void)pInput;
}

//send song position to ui thread to be used in the seekbar
int get_position(){
    int pos = player->position;
    return pos;
}

bool song_is_ended(){ // but the memory lingers on
    bool end_song = player->playback_end;
    return end_song;
}

bool playlist_is_ended(){
    bool end_playlist = player->playlist_end;
    return end_playlist;
}

int get_length(){
    int length = player->song_length;
    return length;
}

int shuffle_toggle(){
    if(atomic_load(&player->shuffle)==false){
        atomic_store(&player->shuffle, true);
        player->shuffle_count = 0;
        shuffle();
        return 1;
    } else {
        atomic_store(&player->shuffle, false);
        return 0;
    }
}

//For values that are only called once when app starts, called from main
void init_player_state(){
    player = (Player* )malloc(sizeof(Player));
    song_info = (MetaInfo* )malloc(sizeof(MetaInfo));
    player->song_total = get_total();
    player->current_index = 0;
    player->shuffle_count = 0;
    if(player->song_total>0){
        create_shuffled_array();
    } else {
        player->shuffled_array=0;
    }
}

//For values that need to be reset between songs
void update_player_state(){
    player->playlist = get_song(player->current_index);
    player->playback_end = false;
    player->playback_stop = false;
    player->playlist_end = false;
    player->is_playing = false;
    player->position=0;
    player->frame=0;
}

int init_playback()
{
    if(!player->playlist) return -666; // dir is empty

    update_player_state();

    if (!player || !player->playlist || !player->playlist->song) {
        printf("No songs to play\n");
        return -1;
    }

    char* filename = player->playlist->song->filename;

    ma_result result;


    result = ma_decoder_init_file(filename, NULL, &decoder);
    if (result != MA_SUCCESS) {
        printf("failed to play track!!!\n");
        return -2;
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &decoder;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        ma_decoder_uninit(&decoder);
        return -3;
    }

    ma_result frameResult = ma_decoder_get_length_in_pcm_frames(&decoder, &player->total_frames);

    player->song_length = (double)player->total_frames / decoder.outputSampleRate;

    int init_info = get_song_metainfo(filename, &song_info);

    player->is_initialized=true;
    return 1;
}

int update_playback(){
    ma_decoder_uninit(&decoder);
    update_player_state();

    ma_result result;

    if(!player||!player->playlist||!player->playlist->song){
        printf("NO SONGS TO PLAY!!");
        return -67;//this was written before the meme was made cringe by boomers
    }

    char* filename = player->playlist->song->filename;

    result = ma_decoder_init_file(filename, NULL, &decoder);
    if (result != MA_SUCCESS) {
        printf("failed to play track!!!\n");
        return -2;
    }

    ma_result frameResult = ma_decoder_get_length_in_pcm_frames(&decoder, &player->total_frames);

    player->song_length = (double)player->total_frames / decoder.outputSampleRate;

    int init_info = get_song_metainfo(filename, &song_info);


    return 1;
}

int play(){
    if(player->is_initialized == false){
        if(init_playback() != 1) return -1;
        update_playback();
    }

    if(player->is_playing == true){
        return 1;
    }

    if(player->playback_stop || player->playlist_end){
        ma_decoder_seek_to_pcm_frame(&decoder, player->frame);
        ma_device_start(&device);
        return 1;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return -4;
    }

    player->is_playing=true;
    return 1;
}

int pause(){
    if(ma_device_stop(&device) != MA_SUCCESS){
        printf("failed to pause track, exiting gracefully!");
        return -69;
    };
    player->is_playing=false;
    return 0;
}

int stop(){
    if(player->playback_stop){
        player->frame=0;
        player->position=0;
        return 1;
    } else {
        if(ma_decoder_seek_to_pcm_frame(&decoder, 0) != MA_SUCCESS){
            printf("failed to start over!");
            return -6;
        };
        player->frame = 0;
        player->position=0;
        ma_device_stop(&device);
        //ma_decoder_uninit(&decoder);
        player->playback_stop = true;
        player->is_playing = false;
    }
    return 0;
}

void next_helper(){
    ma_device_stop(&device);
    update_playback();

    if(atomic_load(&player->shuffle)==true &&
        (player->shuffled_array[player->shuffle_count] ==
            player->shuffled_array[player->song_total-1])){
                player->playlist_end = true;
            } else if((atomic_load(&player->shuffle)==false) &&
                player->current_index == ((player->song_total-1))){
                player->playlist_end = true;
            }


    play();
}

int next(){

    if(atomic_load(&player->shuffle) == true){
        if(player->shuffle_count >= player->song_total ) return 0;
        player -> current_index = player->shuffled_array[player->shuffle_count];
        next_helper();
        player->shuffle_count++;
        return 0;
    }

    if(player->current_index + 1 >= player->song_total ){
        return 1; // do nothing
    }

    player->current_index++;
    next_helper();
    return 0;
}

int prev(){

    if(atomic_load(&player->shuffle) == true){
        if(player->shuffle_count == 0 ) return 0;
        player->shuffle_count--;
        player->current_index = player->shuffled_array[player->shuffle_count];
        next_helper();
        return 0;
    }

    if(player->current_index == 0) {
        return 1;
    }

    player->current_index--;
    next_helper();
    return 0;
}

void player_cleanup(){
    if (player && player->shuffled_array) {
        free(player->shuffled_array);
    }
    if(player){
        free(player);
    }
    if(song_info) free(song_info);
    ma_decoder_uninit(&decoder);
    ma_device_uninit(&device);
}

void seek(int new_position){
    player->frame = (new_position)*(player->total_frames)/100;
    player->position=new_position;
    atomic_store(&player->seek_pending,true);
}

char* get_songname(){
    return player->playlist->song->filename;
}


///////shuffle//Mechanism////////////
void create_shuffled_array(){
    player->shuffled_array = malloc((player->song_total)*sizeof(int));
    if(!player->shuffled_array){
        printf("dammit!!");
        return;
    }
    for(int i = 0; i < player->song_total; i++){
        player->shuffled_array[i]=i;
    }
}

void swap (int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void shuffle(){

    srand ( time(NULL) );

    for (int i = player->song_total-1; i > 0; i--)
        {
            // Pick a random index from 0 to i
            int j = rand() % (i+1);

            // Swap arr[i] with the element at random index
            swap(&player->shuffled_array[i], &player->shuffled_array[j]);
        }
}

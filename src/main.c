#include "../include/playlist.h"
#include "../include/player.h"
#include "../include/ui.h"
#include <unistd.h>


int main(int argc, char* argv[]){


    build_playlist_from_dir();
    init_player_state();
    init_playback();

    ui_main(0, NULL);

    playlist_cleanup();
    player_cleanup();

}

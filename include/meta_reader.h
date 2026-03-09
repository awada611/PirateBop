#ifndef META_READER_H
#define META_READER_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

typedef struct MetaInfo{
    char* title;
    char* artist;
    char* album;
    char* track_number;
    uint8_t *cover_ptr;
    size_t cover_size;
}MetaInfo;

int get_song_metainfo(char* filename, MetaInfo** song_info);

#endif

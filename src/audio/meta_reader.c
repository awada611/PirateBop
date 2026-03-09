
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <stdint.h>
#include <string.h>

#include "../../include/meta_reader.h"


struct buffer_data {
    uint8_t *ptr;
    size_t size; ///< size left in the buffer
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    if (!buf_size)
        return AVERROR_EOF;
    //printf("ptr:%p size:%zu\n", bd->ptr, bd->size);

    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;

    return buf_size;
}

int get_song_metainfo(char* filename, MetaInfo** song_info)
{
    AVFormatContext *fmt_ctx = NULL;
    AVDictionaryEntry *tag = NULL;
    AVIOContext *avio_ctx = NULL;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
    char *input_filename = NULL;
    int ret = 0;
    struct buffer_data bd = { 0 };

    input_filename = filename;

    /* slurp file content into buffer */
    ret = av_file_map(input_filename, &buffer, &buffer_size, 0, NULL);
    if (ret < 0)
        goto end;

    /* fill opaque structure used by the AVIOContext read callback */
    bd.ptr  = buffer;
    bd.size = buffer_size;

    if (!(fmt_ctx = avformat_alloc_context())) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
                                  0, &bd, &read_packet, NULL, NULL);
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    fmt_ctx->pb = avio_ctx;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input\n");
        goto end;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto end;
    }

    //
    tag = av_dict_get(fmt_ctx->metadata, "title", NULL, 0);
    (*song_info)->title = tag ? strdup(tag->value) : strdup("Unknown Title");
    tag = av_dict_get(fmt_ctx->metadata, "artist", NULL, 0);
    (*song_info)->artist = tag ? strdup(tag->value) : strdup("Unknown Artist");
    tag = av_dict_get(fmt_ctx->metadata, "album", NULL, 0);
    (*song_info)->album = tag ? strdup(tag->value) :strdup ("Unknown Album");
    tag = av_dict_get(fmt_ctx->metadata, "track", NULL, 0);
    (*song_info)->track_number = tag ? strdup(tag->value) :strdup ("");






    (*song_info)->cover_ptr=NULL;
    (*song_info)->cover_size=0;

    for(int i = 0; i < fmt_ctx->nb_streams; i++){
        if(fmt_ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC){
            AVPacket *img_packet = &fmt_ctx->streams[i]->attached_pic;
            (*song_info)->cover_ptr = (uint8_t*)malloc(img_packet->size);
            if((*song_info)->cover_ptr){
                memcpy((*song_info)->cover_ptr, img_packet->data, img_packet->size);
                (*song_info)->cover_size = img_packet->size;
            }
            break;
        }else {
            if((*song_info)->cover_ptr){
                free((*song_info)->cover_ptr);
                (*song_info)->cover_ptr=NULL;
                (*song_info)->cover_size=0;
            }
        }
    }


    //av_dump_format(fmt_ctx, 0, input_filename, 0);

end:
    avformat_close_input(&fmt_ctx);

    /* note: the internal buffer could have changed, and be != avio_ctx_buffer */
    if (avio_ctx)
        av_freep(&avio_ctx->buffer);
    avio_context_free(&avio_ctx);

    av_file_unmap(buffer, buffer_size);

    if (ret < 0) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}

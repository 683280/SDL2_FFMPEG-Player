//
// Created by CarlJay on 2018/1/23.
//

#ifndef FFMPEG_FFMPEG_OBJECT_H
#define FFMPEG_FFMPEG_OBJECT_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "ffmpeg_player.h"
#include "codec_object.h"
typedef struct {
    CodecObject* video_codec;
    CodecObject* audio_codec;
    MediaPlayer* player;
    AVFormatContext* formatCtx;
    //queue
    QueuePacket queue_audio_packet;
    QueuePacket queue_video_packet;
    QueueFrame queue_audio_frame;
    QueueFrame queue_video_frame;

    int video_stream_idx,audio_stream_idx;
    const char* uri;
    //显示的宽高
    int width,height;
    //IO超时
    int timeout;
}FFMPEG_Object;

int init_ffmpeg();
FFMPEG_Object* create_ffmpeg_object();
int destroy_ffmpeg_object(FFMPEG_Object* ffmpeg_object);
int set_uri(FFMPEG_Object* ffmpeg_object,const char* uri);
int seek(FFMPEG_Object* ffmpeg_object,int second);
int init_player(FFMPEG_Object* object,int width,int height);

int start(FFMPEG_Object* ffmpeg_object);

static int get_decodec_ctx(AVFormatContext* formatCtx, enum AVMediaType type,AVCodecContext** video_codec_ctx,int* index);

int open_uri(FFMPEG_Object* ffmpeg_object);
int read_thread(void* pVoid);
int decode_frame(AVCodecContext* codecCtx,AVFrame* frame,QueueFrame*queueFrame);
#endif //FFMPEG_FFMPEG_OBJECT_H

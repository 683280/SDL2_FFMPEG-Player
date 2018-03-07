//
// Created by 13342 on 2018/3/4.
//

#ifndef FFMPEG_PLAYER_CODEC_OBJECT_H
#define FFMPEG_PLAYER_CODEC_OBJECT_H

#include <libavcodec/avcodec.h>
#include <queue.h>

typedef struct {
    QueueFrame* queueFrame;
    QueuePacket* queuePacket;
    AVCodecContext* mCodecCtx;
    int64_t next_pts;
    AVRational next_pts_tb;
}CodecObject;
int create_codec_object(CodecObject** pVideoCodec,AVCodecContext*video_codec_ctx,QueuePacket*queuePacket,QueueFrame*queueFrame);
int destroy_codec_object(CodecObject* object);
int decodec_codec_thread(void* data);
int decoder_decode_frame(CodecObject* object,AVFrame* frame);
#endif //FFMPEG_PLAYER_CODEC_OBJECT_H

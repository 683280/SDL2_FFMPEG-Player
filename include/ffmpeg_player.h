//
// Created by CarlJay on 2018/2/22.
//

#ifndef FFMPEG_PLAYER_FFMPEG_PLAYER_H
#define FFMPEG_PLAYER_FFMPEG_PLAYER_H
#include <libavutil/frame.h>
#include <SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <error.h>
#include "log.h"
#include <queue.h>
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)
typedef struct {
    int width;
    int height;
    AVCodecContext* audio_codec_ctx;
    //SDL2
    SDL_Renderer *sdlRenderer;
    SDL_Texture *sdlTexture;
    SDL_Rect sdlRect;
    //queue
    QueueFrame* videoQueue;
    QueueFrame* audioQueue;
    //转码
    AVFrame* sws_frame,*swr_frame;
    struct SwsContext* sws_ctx;
    struct SwrContext* swr_ctx;
    //音频
    uint8_t* buf_data;
    int buf_size;
    int buf_index;
    //音视频同步
    AVStream* video_st,*audio_st;
    double video_clock;//已经播放的时常
    double frame_last_delay,frame_last_pts,audio_clock,frame_timer;
}MediaPlayer;
int init_sdl(MediaPlayer* player,int width,int height);
int init_sws(MediaPlayer*player,AVCodecContext* codec_ctx,int width,int height);
int init_swr(MediaPlayer*player,AVCodecContext* codec_ctx);
int get_sws_frame(MediaPlayer* player);

void fill_audio(void *udata,Uint8 *stream,int len);
uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque);
void schedule_refresh(MediaPlayer* media, int delay);
int video_refresh_timer(MediaPlayer*player);
int play(MediaPlayer* player);
#endif //FFMPEG_PLAYER_FFMPEG_PLAYER_H

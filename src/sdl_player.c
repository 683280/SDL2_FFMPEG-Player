//
// Created by CarlJay on 2018/2/22.
//

#include <ffmpeg_player.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>

static SDL_RendererInfo renderer_info = {0};
int init_sdl(MediaPlayer* player,int width,int height){
    SDL_Window *window;
    SDL_Renderer *sdlRenderer;
    SDL_Texture *sdlTexture;
    SDL_Rect sdlRect;
    int flag;
    //SDL 初始化视频 音频 定时器
    flag = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_AUDIO;
    if(SDL_Init(flag)){
        log("SDL init error%s",SDL_GetError());
        return ERROR_SDL_INIT;
    }

    //SDL
    window = SDL_CreateWindow("FFMPEG Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window){
        return ERROR_SDL_INIT;
    }
    //使用硬件加速  使用和窗口的同步频率
    flag = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    sdlRenderer = SDL_CreateRenderer(window, -1, flag);
    if(!sdlRenderer){
        log("无法使用硬件加速或者窗口同步频率\n");
        sdlRenderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (sdlRenderer) {
        if (!SDL_GetRendererInfo(sdlRenderer, &renderer_info))
            log("Initialized %s renderer.\n", renderer_info.name);
    }
    if (!window || !sdlRenderer || !renderer_info.num_texture_formats) {
        log("Failed to create window or renderer: %s", SDL_GetError());
        return ERROR_SDL_INIT;
    }
    //创建纹理（Texture）
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    //FIX: If window is resize
    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = width;
    sdlRect.h = height;
    player->sdlRenderer = sdlRenderer;
    player->sdlTexture = sdlTexture;
    player->sdlRect = sdlRect;

    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = 44100;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 1;
    wanted_spec.silence = 0;
    wanted_spec.samples = 1024;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = player;
    if (SDL_OpenAudio(&wanted_spec, NULL)<0){
        log("can't open audio.\n");
        return -1;
    }
    if(SDL_RegisterEvents(3) == -1){
        exit(0);
    }
    return 0;
}
int init_sws(MediaPlayer*player,AVCodecContext* codec_ctx,int width,int height){
    player->width = width;
    player->height = height;
    //创建并初始化sws 图像格式转换
    struct SwsContext * sws_ctx = sws_getContext(codec_ctx->width,
                                                 codec_ctx->height,
                                                 codec_ctx->pix_fmt,
                                                 width, height,
                                                 AV_PIX_FMT_YUV420P,
                                                 SWS_BILINEAR, NULL, NULL, NULL);
    if(!sws_ctx)
        return -3;
    AVFrame* sws_frame = av_frame_alloc();
    if(!sws_frame)
        return -4;
    int image_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1) * sizeof(uint8_t);
    uint8_t *buffer = (av_malloc(image_size));
    if(!buffer)
        return -5;
    av_image_fill_arrays(sws_frame->data, sws_frame->linesize, buffer, AV_PIX_FMT_YUV420P, width, height, 1);
    player->sws_ctx = sws_ctx;
    player->sws_frame = sws_frame;
}
int init_swr(MediaPlayer*player,AVCodecContext* codec_ctx){
    player->audio_codec_ctx = codec_ctx;
    SwrContext* swr_ctx = swr_alloc_set_opts(NULL,
                                             1,//输出通道数
                                             AV_SAMPLE_FMT_S16,//输出格式
                                             44100,//输出采集率
                                             codec_ctx->channel_layout,
                                             codec_ctx->sample_fmt,
                                             codec_ctx->sample_rate,
                                             0,
                                             0);
    swr_init(swr_ctx);
    player->swr_ctx = swr_ctx;
    player->buf_data = malloc(8016);
}
double synchronize(MediaPlayer *player,AVFrame *srcFrame, double pts)
{
    double frame_delay;

    if (pts != 0)
        player->video_clock = pts; // Get pts,then set video clock to it
    else
        pts = player->video_clock; // Don't get pts,set it to video clock

    frame_delay = av_q2d(player->video_st->time_base);
    frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

    player->video_clock += frame_delay;

    return pts;
}
int get_sws_frame(MediaPlayer* player){
    AVFrame* vframe = get_queue_frame(player->videoQueue);
    double pts;
    if((pts = av_frame_get_best_effort_timestamp(vframe)) == AV_NOPTS_VALUE)
        pts = 0;
    double d = av_q2d(player->video_st->time_base);
    pts *= d;
    pts = synchronize(player,vframe,pts);
    AVFrame* sws_frame = player->sws_frame;
    sws_frame->opaque = &pts;
    int i = sws_scale(player->sws_ctx,
                      (const uint8_t *const *) (vframe->data),
                      vframe->linesize, 0, vframe->height,
                      sws_frame->data, sws_frame->linesize);
//    av_frame_free(vframe);

    return 0;
}
int get_swr_frame(MediaPlayer*player,uint8_t* audio_buf){
    int size;
    AVCodecContext* audio_codec_ctx = player->audio_codec_ctx;

    AVFrame* aframe = get_queue_frame(player->audioQueue);
    //转码
    if (audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_S16P) {
        size = av_samples_get_buffer_size(aframe->linesize, audio_codec_ctx->channels,
                                          audio_codec_ctx->frame_size, audio_codec_ctx->sample_fmt, 1);
    } else {
        av_samples_get_buffer_size(&size, audio_codec_ctx->channels, audio_codec_ctx->frame_size,
                                   audio_codec_ctx->sample_fmt, 1);
    }
//    if (player->buf_size < size){
//        if(player->buf_data != NULL)
//            player->buf_data = realloc(player->buf_data,size);
//        else
//            player->buf_data = malloc(size);
//    }
    int size1 = swr_convert(player->swr_ctx, &player->buf_data, size, (uint8_t const **) (aframe->data),
                            aframe->nb_samples);
    int64_t pts = av_frame_get_best_effort_timestamp(aframe) ;
    player->audio_clock = pts * av_q2d(player->audio_st->time_base);
//    av_frame_free(aframe);
    int data_size =  size1 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    return data_size;//data_size;
}
void fill_audio(void *udata,Uint8 *stream,int len){
    //SDL 2.0
    MediaPlayer* player = udata;
    SDL_memset(stream, 0, len);
    int audio_size;
    int len1;
    while(len > 0)
    {
        if (player->buf_index >= player->buf_size)
        {
            //数据全部发送，再去获取
            //自定义的一个函数
            audio_size = get_swr_frame(player, player->buf_data);
            if (audio_size < 0)
            {
                //错误则静音
                player->buf_size = 1024;
                memset(player->buf_data, 0, player->buf_size);
            }
            else
            {
                player->buf_size = audio_size;
            }
            player->buf_index = 0;      //回到缓冲区开头
        }

        len1 = player->buf_size - player->buf_index;
        if (len1 >= len)       //len1常比len大，但是一个callback只能发len个
        {
            SDL_MixAudio(stream, player->buf_data + player->buf_index, len, SDL_MIX_MAXVOLUME);
            player->buf_index += len;
            return;
        }
        //新程序用 SDL_MixAudioFormat()代替
        //混合音频， 第一个参数dst， 第二个是src，audio_buf_size每次都在变化
        SDL_MixAudio(stream, player->buf_data + player->buf_index, len1, SDL_MIX_MAXVOLUME);
        //
        //memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        player->buf_index += len1;
    }
}
void schedule_refresh(MediaPlayer* media, int delay){
    SDL_AddTimer(delay, sdl_refresh_timer_cb, media);
}
uint32_t sdl_refresh_timer_cb(uint32_t interval, void *opaque)
{
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = opaque;
    SDL_PushEvent(&event);
    return 0; /* 0 means stop timer */
}
static const double SYNC_THRESHOLD = 0.01;
static const double NOSYNC_THRESHOLD = 10.0;
int video_refresh_timer(MediaPlayer*player){
    get_sws_frame(player);
    double  current_pts = *(double*)player->sws_frame->opaque;
    double  delay = current_pts -player->frame_last_pts;
    if (delay <= 0 || delay >= 1.0)
        delay = player->frame_last_delay;

    player->frame_last_delay = delay;
    player->frame_last_pts = current_pts;

    double ref_clock = player->audio_clock;
    double diff = current_pts - ref_clock;
    double threshold = delay;//(delay > SYNC_THRESHOLD) ? delay : SYNC_THRESHOLD;
    double ret_delay;
    if (diff <= -threshold){//慢了 加快速度
        ret_delay = delay / 2;
//        if (diff <= - threshold)//慢了,delay设为0
//            delay = 0;
//        else if(diff >= threshold)//快了,加快delay
//            delay *= 2;
    }else if(diff >= threshold){//快了,在上一帧延迟的基础上加
        ret_delay = delay * 2;
    } else{
        ret_delay = delay;
    }
//    player->frame_timer += delay;
//    double  actual_delay = player->frame_timer - (double)(av_gettime()) / 1000000.0;
//    if (actual_delay <= 0.010)
//        actual_delay = 0.010;
    schedule_refresh(player, (int)(ret_delay * 1000));
    //设置纹理的数据
    SDL_UpdateTexture(player->sdlTexture, NULL, player->sws_frame->data[0], player->width);
    SDL_RenderClear(player->sdlRenderer);
    //将纹理的数据拷贝给渲染器
    SDL_RenderCopy(player->sdlRenderer, player->sdlTexture, NULL, &player->sdlRect);
    //显示
    SDL_RenderPresent(player->sdlRenderer);
}
int play(MediaPlayer* player){
    SDL_PauseAudio(0);
    schedule_refresh(player,40);
    SDL_Event event;
    printf("FF_REFRESH_EVENT = %d", FF_REFRESH_EVENT);
    while(1){
        SDL_WaitEvent(&event);
        switch( event.type ) {
            case FF_QUIT_EVENT:
            case SDL_QUIT:
                SDL_Quit();//退出系统
                return 0;
            case FF_REFRESH_EVENT:
                video_refresh_timer(player);
                break;
            default:
                break;
        }
    }
}
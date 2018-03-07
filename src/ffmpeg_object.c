//
// Created by CarlJay on 2018/1/23.
//

#include <ffmpeg_object.h>


int init_ffmpeg() {
    av_register_all();
    avformat_network_init();
    return 0;
}
int init_player(FFMPEG_Object* object,int width,int height){
    object->width = width;
    object->height = height;
    init_sdl(object->player,width,height);
}
FFMPEG_Object *create_ffmpeg_object() {
    FFMPEG_Object* ffmpeg_object = malloc(sizeof(FFMPEG_Object));
    if(ffmpeg_object == NULL){
        return NULL;
    }
    memset(ffmpeg_object,0, sizeof(FFMPEG_Object));

    //packet queue init
    init_packet_queue(&ffmpeg_object->queue_audio_packet,100);
    init_packet_queue(&ffmpeg_object->queue_video_packet,100);
    init_frame_queue(&ffmpeg_object->queue_audio_frame,50);
    init_frame_queue(&ffmpeg_object->queue_video_frame,20);

    //FFMPEG init
    AVFormatContext *formatCtx = avformat_alloc_context();
//    formatCtx->interrupt_callback.callback = interrupt_cb;
//    formatCtx->interrupt_callback.opaque = ffmpeg_object;
    ffmpeg_object->formatCtx = formatCtx;
    MediaPlayer* player = malloc(sizeof(MediaPlayer));
    memset(player,0, sizeof(MediaPlayer));
    player->buf_data = malloc(8096);
    ffmpeg_object->player = player;
    player->audioQueue = &ffmpeg_object->queue_audio_frame;
    player->videoQueue = &ffmpeg_object->queue_video_frame;
    return ffmpeg_object;
}
int destroy_ffmpeg_object(FFMPEG_Object *ffmpeg_object) {
    free(ffmpeg_object);
    return 0;
}
int set_uri(FFMPEG_Object* ffmpeg_object,const char* uri){
    if(uri == NULL)
        return -1;
    ffmpeg_object->uri = av_strdup(uri);
    return 0;
}
int start(FFMPEG_Object* ffmpeg_object){
    int ret = open_uri(ffmpeg_object);
    if(ret < 0){
        return -1;
    }
    SDL_CreateThread(read_thread,"read_thread",ffmpeg_object);
    SDL_CreateThread(decodec_codec_thread,"decodec_video_thread",ffmpeg_object->video_codec);
    SDL_CreateThread(decodec_codec_thread,"decodec_audio_thread",ffmpeg_object->audio_codec);
    SDL_CreateThread(play,"play",ffmpeg_object->player);
//    play(ffmpeg_object->player);
    return 0;
}
int seek(FFMPEG_Object* ffmpeg_object,int second){
//    av_seek_frame(ffmpeg_object->formatCtx,ffmpeg_object->video_stream_idx, second*vid_time_scale/time_base, AVSEEK_FLAG_BACKWARD);
//    AVStream* vs = ffmpeg_object->formatCtx->streams[ffmpeg_object->audio_stream_idx];
//    av_seek_frame(ffmpeg_object->formatCtx,ffmpeg_object->audio_stream_idx, second*1/av_q2d(vs->time_base), AVSEEK_FLAG_BACKWARD);
    av_seek_frame(ffmpeg_object->formatCtx,-1, second * AV_TIME_BASE, AVSEEK_FLAG_ANY);
    queue_packet_clean(&ffmpeg_object->queue_audio_packet);
    queue_packet_clean(&ffmpeg_object->queue_video_packet);
    queue_frame_clean(&ffmpeg_object->queue_audio_frame);
    queue_frame_clean(&ffmpeg_object->queue_video_frame);
}
static int interrupt_cb(void *ctx)
{
    printf("interrupt_cb");
    return 0;
}

static int get_decodec_ctx(AVFormatContext* formatCtx, enum AVMediaType type,AVCodecContext** video_codec_ctx,int* index){
    int i = av_find_best_stream(formatCtx,type,-1,-1,NULL,0);
    AVCodecParameters * codecpar = formatCtx->streams[i]->codecpar;
    AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    *video_codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(*video_codec_ctx,codecpar);
    avcodec_open2(*video_codec_ctx,codec,NULL);
    *index = i;
}
int open_uri(FFMPEG_Object* ffmpeg_object){
    int ret;
    AVFormatContext* formatCtx = ffmpeg_object->formatCtx;
    ret = avformat_open_input(&formatCtx, ffmpeg_object->uri, NULL, NULL);
    if(!formatCtx || ret < 0){
        log("avformat_open_input error:av_format = NULL,error = %d\n",ret);
        return -1;
    }
    //打印视频参数
    av_dump_format(formatCtx,0,ffmpeg_object->uri,0);
    avformat_find_stream_info(formatCtx, NULL);
    AVCodecContext*video_codec_ctx,*audio_codec_ctx;
    int video_stream_idx,audio_stream_idx;
    get_decodec_ctx(formatCtx,AVMEDIA_TYPE_VIDEO,&video_codec_ctx,&video_stream_idx);
    get_decodec_ctx(formatCtx,AVMEDIA_TYPE_AUDIO,&audio_codec_ctx,&audio_stream_idx);

    if (video_codec_ctx != NULL){
        ret = create_codec_object(&ffmpeg_object->video_codec,
                                 video_codec_ctx,
                                 &ffmpeg_object->queue_video_packet,
                                 &ffmpeg_object->queue_video_frame);
        if (ret < 0)
            return -1;
        ffmpeg_object->video_stream_idx = video_stream_idx;
        init_sws(ffmpeg_object->player,video_codec_ctx,ffmpeg_object->width,ffmpeg_object->height);
        ffmpeg_object->player->video_st = formatCtx->streams[video_stream_idx];
    }
    if (audio_codec_ctx != NULL){
        ret = create_codec_object(&ffmpeg_object->audio_codec,
                                 audio_codec_ctx,
                                 &ffmpeg_object->queue_audio_packet,
                                 &ffmpeg_object->queue_audio_frame);
        if (ret < 0)
            return -1;
        ffmpeg_object->audio_stream_idx = audio_stream_idx;
        init_swr(ffmpeg_object->player,audio_codec_ctx);
        ffmpeg_object->player->audio_st = formatCtx->streams[audio_stream_idx];
    }
    return 0;
}
/**
 *  read packet thread
 * @param ffmpeg_object
 */
int read_thread(void* pVoid){
    FFMPEG_Object* ffmpeg_object = pVoid;
    int ret;
    AVFormatContext *ic = ffmpeg_object->formatCtx;
    SDL_mutex *wait_mutex = SDL_CreateMutex();
    if (!wait_mutex) {
        log("SDL_CreateMutex(): %s\n", SDL_GetError());
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    if (ret < 0)
        goto fail;
    log("ffmpeg_start\n");
    AVFormatContext* formatCtx = ffmpeg_object->formatCtx;
    int video_stream_idx = ffmpeg_object->video_stream_idx;
    int audio_stream_idx = ffmpeg_object->audio_stream_idx;
    log("reading packet\n");
    AVPacket pkt;
    for(;;){
        if ((ret = av_read_frame(formatCtx, &pkt)) < 0) {
            break;
        }
        if (pkt.stream_index == video_stream_idx) {
            queue_put_packet(&ffmpeg_object->queue_video_packet,&pkt);
        } else if(pkt.stream_index == audio_stream_idx){
            queue_put_packet(&ffmpeg_object->queue_audio_packet,&pkt);
        }
        av_packet_unref(&pkt);
    }
    fail:
    if (ic)
        avformat_close_input(&ic);

    if (ret != 0) {
        SDL_Event event;
        event.type = -1;
        event.user.data1 = ffmpeg_object;
        SDL_PushEvent(&event);
    }
    SDL_DestroyMutex(wait_mutex);
}
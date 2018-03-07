//
// Created by 13342 on 2018/3/4.
//

#include <codec_object.h>

int create_codec_object(CodecObject** pCodecObject,AVCodecContext*video_codec_ctx,QueuePacket* queuePacket,QueueFrame*queueFrame){
    if(!video_codec_ctx)
        return -1;
    CodecObject* codecObject = malloc(sizeof(CodecObject));
    if(!codecObject)
        return -2;
    codecObject->mCodecCtx = video_codec_ctx;
    codecObject->queueFrame = queueFrame;
    codecObject->queuePacket = queuePacket;

    //创建视频队列
    *pCodecObject = codecObject;
    return 0;
}
int destroy_codec_object(CodecObject *object) {
    av_free(object->mCodecCtx);
    free(object);
    return 0;
}
int decodec_codec_thread(void* data){
    CodecObject* object = data;
    AVFrame* frame = av_frame_alloc();
    int ret;
    for(;;){
        ret = decoder_decode_frame(object,frame);
        if(ret < 1)
            return ret;
        queue_put_frame(object->queueFrame,av_frame_clone(frame));
        av_frame_unref(frame);
    }
}
int decoder_decode_frame(CodecObject* object,AVFrame* frame){
    int ret = AVERROR(EAGAIN);
    for(;;){
        do {
            switch (object->mCodecCtx->codec_type) {
                case AVMEDIA_TYPE_VIDEO:
                    ret = avcodec_receive_frame(object->mCodecCtx, frame);
                    if (ret >= 0) {
                        frame->pts = frame->best_effort_timestamp;
//                    frame->pts = frame->pkt_dts;
                    }
                    break;
                case AVMEDIA_TYPE_AUDIO:
                    ret = avcodec_receive_frame(object->mCodecCtx, frame);
                    if (ret >= 0) {
                        AVRational tb = (AVRational) {1, frame->sample_rate};
                        if (frame->pts != AV_NOPTS_VALUE)
                            frame->pts = av_rescale_q(frame->pts, object->mCodecCtx->pkt_timebase, tb);
                        else if (object->next_pts != AV_NOPTS_VALUE)
                            frame->pts = av_rescale_q(object->next_pts, object->next_pts_tb, tb);
                        if (frame->pts != AV_NOPTS_VALUE) {
                            object->next_pts = frame->pts + frame->nb_samples;
                            object->next_pts_tb = tb;
                        }
                    }
                    break;
            }
            if (ret >= 0)
                return 1;
        }while (ret != AVERROR(EAGAIN));
        AVPacket* packet = get_queue_packet(object->queuePacket);
        avcodec_send_packet(object->mCodecCtx, packet);
        av_packet_free(&packet);
    }
}
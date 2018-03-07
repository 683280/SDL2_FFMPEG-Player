//
// Created by CarlJay on 2018/1/23.
//

#ifndef FFMPEG_QUEUE_H
#define FFMPEG_QUEUE_H
#define FRAME_MAX_SIZE 16
#include <stdlib.h>
#include <libavutil/frame.h>
#include <SDL2/SDL_mutex.h>
#include <libavcodec/avcodec.h>
typedef struct FrameNode{
    AVFrame* frame;
    struct FrameNode* next;
}FrameNode;

typedef struct {
    FrameNode* first;
    FrameNode* last;
    SDL_cond* cond;
    SDL_mutex* mutex;
    int size;
    int max_size;
}QueueFrame;
typedef struct PacketNode{
    AVPacket* pkt;
    struct PacketNode* next;
}PacketNode;
typedef struct {
    PacketNode* first;
    PacketNode* last;
    SDL_cond* cond;
    SDL_mutex* mutex;
    int size;
    int max_size;
}QueuePacket;

//Packet
int init_packet_queue(QueuePacket* queueFrame,int max_size);
int queue_put_packet(QueuePacket* queue,AVPacket*pkt);
AVPacket* get_queue_packet(QueuePacket* queue);
int queue_packet_clean(QueuePacket* queue);
//Frame
int init_frame_queue(QueueFrame* queueFrame,int max_size);
int queue_put_frame(QueueFrame* queue,AVFrame* frame);
AVFrame* get_queue_frame(QueueFrame* queue);
int queue_frame_clean(QueueFrame* queue);
#endif //FFMPEG_QUEUE_H

//
// Created by 豪杰 on 2017/1/13.
//
#include <queue.h>
#include "log.h"
int init_frame_queue(QueueFrame* queueFrame,int max_size){
    memset(queueFrame,0, sizeof(QueueFrame));
    queueFrame->mutex = SDL_CreateMutex();
    queueFrame->cond = SDL_CreateCond();
    queueFrame->max_size = max_size;
    return 0;
}
int queue_put_frame(QueueFrame* queue,AVFrame* frame){

    SDL_LockMutex(queue->mutex);
    if (queue->size == queue->max_size){
        SDL_CondWait(queue->cond,queue->mutex);
    }
    FrameNode* node = malloc(sizeof(FrameNode));
    memset(node,0, sizeof(FrameNode));
    node->frame = frame;
    if(queue->first == NULL){
        queue->first = node;
    }
    if (queue->last == NULL){
        queue->last = node;
        goto end;
    }
    queue->last->next = node;
    queue->last = node;
    end:
    queue->size++;
    SDL_CondSignal(queue->cond);
    SDL_UnlockMutex(queue->mutex);
}
AVFrame* get_queue_frame(QueueFrame* queue){
    SDL_LockMutex(queue->mutex);
    if (queue->size == 0){
        SDL_CondWait(queue->cond,queue->mutex);
    }
    FrameNode* node = queue->first;
    queue->first = node->next;
    queue->size--;
    if (queue->first == NULL){
        queue->last = NULL;
    }
    SDL_CondSignal(queue->cond);
    SDL_UnlockMutex(queue->mutex);
    return node->frame;
}
int queue_frame_clean(QueueFrame* queue){
    SDL_LockMutex(queue->mutex);
    FrameNode* node,*next;
    next = queue->first;
    for (;next != NULL;) {
        av_frame_free(&next->frame);
        node = next;
        next = node->next;
        free(node);
    }
    queue->size = 0;
    queue->first = NULL;
    queue->last = NULL;
    SDL_UnlockMutex(queue->mutex);
}
int init_packet_queue(QueuePacket* queueFrame,int max_size){
    memset(queueFrame,0, sizeof(QueuePacket));
    queueFrame->mutex = SDL_CreateMutex();
    queueFrame->cond = SDL_CreateCond();
    queueFrame->max_size = max_size;
    return 0;
}
int queue_put_packet(QueuePacket* queue,AVPacket*pkt){

    SDL_LockMutex(queue->mutex);
    if (queue->size == queue->max_size){
        SDL_CondWait(queue->cond,queue->mutex);
    }
    PacketNode* node = malloc(sizeof(PacketNode));
    memset(node,0, sizeof(PacketNode));
    node->pkt = av_packet_clone(pkt);
    if(queue->first == NULL){
        queue->first = node;
    }
    if (queue->last == NULL){
        queue->last = node;
        goto end;
    }
    queue->last->next = node;
    queue->last = node;
    end:
    queue->size++;
    SDL_CondSignal(queue->cond);
    SDL_UnlockMutex(queue->mutex);
}
AVPacket* get_queue_packet(QueuePacket* queue){
    SDL_LockMutex(queue->mutex);
    if (queue->size == 0){
        SDL_CondWait(queue->cond,queue->mutex);
    }
    PacketNode* node = queue->first;
    queue->first = node->next;
    queue->size--;
    SDL_CondSignal(queue->cond);
    SDL_UnlockMutex(queue->mutex);
    return node->pkt;
}
int queue_packet_clean(QueuePacket* queue){
    SDL_LockMutex(queue->mutex);
    PacketNode* node,*next;
    next = queue->first;
    for (;next != NULL;) {
        av_packet_free(&next->pkt);
        node = next;
        next = node->next;
        free(node);
    }
    queue->size = 0;
    queue->first = NULL;
    queue->last = NULL;
    SDL_UnlockMutex(queue->mutex);
}

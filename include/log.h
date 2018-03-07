//
// Created by CarlJay on 2018/1/23.
//

#ifndef FFMPEG_PLAYER_LOG_H
#define FFMPEG_PLAYER_LOG_H

#include <stdio.h>
#define log(a,...) printf(a,##__VA_ARGS__)
#endif //FFMPEG_PLAYER_LOG_H

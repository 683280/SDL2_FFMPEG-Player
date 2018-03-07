cmake_minimum_required(VERSION 3.9.6)
project(ffmpeg_player)

set(CMAKE_C_STANDARD 99)
include_directories(include)

set(SOURCE_FILE main.c src/ffmpeg_object.c src/sdl_player.c src/codec_object.c src/queue.c )
add_executable(ffmpeg_player ${SOURCE_FILE})
target_link_libraries(ffmpeg_player avformat avutil avcodec swscale swresample SDL2main SDL2)

#include <ffmpeg_object.h>
#include <afxres.h>
#ifdef LINUX
int main()
#elif WIN32
int WINAPI WinMain(HINSTANCE hInstance,      // handle to current instance
            HINSTANCE hPrevInstance,  // handle to previous instance
            LPSTR lpCmdLine,          // command line
            int nCmdShow)
#endif
{
    int width = 480;
    int height = 320;
//    const char *url = "../test.mp4";
    const char *url = "http://221.228.226.23/11/t/j/v/b/tjvbwspwhqdmgouolposcsfafpedmb/sh.yinyuetai.com/691201536EE4912BF7E4F1E2C67B8119.mp4";
//    const char *url = "rtmp://pull-g.kktv8.com/livekktv/100987038 ";
//    const char *url = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
//    const char *url = "http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8v";
    init_ffmpeg();
    FFMPEG_Object* ffmpeg_object = create_ffmpeg_object();
    set_uri(ffmpeg_object,url);
    init_player(ffmpeg_object,width,height);
    start(ffmpeg_object);
    while(1){
        getchar();
        seek(ffmpeg_object,20);
    }
    return 0;
}

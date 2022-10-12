#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>



#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
    
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
//#include <libavutil/frame.h>
#include <SDL2/SDL.h>
};

/*
ffmpeg path: d:/dev/libffmpeg
sdl2 path: d:/dev/sdl2

to compile it:
gcc -Id:/dev/libffmpeg/include -Id:/dev/sdl2/include \
    -Ld:/dev/libffmpeg/lib -Ld:/dev/sdl2/lib \
    fplay.c -lavcodec -lavutil -lavformat -lmingw32 -lSDL2main -lSDL2
*/

void display(AVCodecContext*, AVPacket*, AVFrame*, SDL_Rect*,
    SDL_Texture*, SDL_Renderer*, double);

void playaudio(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame,
    SDL_AudioDeviceID auddev);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    bool running = true;
    uint32_t windowID = 0;
    bool foundVideo = false, foundAudio = false;
    // ffmpeg part
    AVFormatContext *pFormatCtx;
    int vidId = -1, audId = -1;
    double fpsrendering = 0.0;
    AVCodecContext *vidCtx, *audCtx;
    AVCodec *vidCodec, *audCodec;
    AVCodecParameters *vidpar, *audpar;
    AVFrame *vframe, *aframe;
    AVPacket *packet;

    //sdl part
    int swidth, sheight;
    SDL_Window *screen;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_AudioDeviceID auddev;
    SDL_AudioSpec want, have;

    SDL_Init(SDL_INIT_EVERYTHING);
    pFormatCtx = avformat_alloc_context();
    char bufmsg[1024];
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) < 0) {
        sprintf(bufmsg, "Cannot open %s", argv[1]);
        perror(bufmsg);
        goto clean_format_context;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        perror("Cannot find stream info. Quitting.");
        goto clean_format_context;
    }
  
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        AVCodecParameters *localparam = pFormatCtx->streams[i]->codecpar;
        AVCodec *localcodec = avcodec_find_decoder(localparam->codec_id);
        if (localparam->codec_type == AVMEDIA_TYPE_VIDEO && !foundVideo) {
            vidCodec = localcodec;
            vidpar = localparam;
            vidId = i;
            AVRational rational = pFormatCtx->streams[i]->avg_frame_rate;
            fpsrendering = 1.0 / ((double)rational.num / (double)(rational.den));
            foundVideo = true;
        } else if (localparam->codec_type == AVMEDIA_TYPE_AUDIO && !foundAudio) {
            audCodec = localcodec;
            audpar = localparam;
            audId = i;
            foundAudio = true;
        }
        if (foundVideo && foundAudio) { break; }
    }
    vidCtx = avcodec_alloc_context3(vidCodec);
    audCtx = avcodec_alloc_context3(audCodec);
    if (avcodec_parameters_to_context(vidCtx, vidpar) < 0) {
        perror("vidCtx");
        goto clean_codec_context;
    }
    if (avcodec_parameters_to_context(audCtx, audpar) < 0) {
        perror("audCtx");
        goto clean_codec_context;
    }
    if (avcodec_open2(vidCtx, vidCodec, NULL) < 0) {
        perror("vidCtx");
        goto clean_codec_context;
    }
    if (avcodec_open2(audCtx, audCodec, NULL) < 0) {
        perror("audCtx");
        goto clean_codec_context;
    }

    vframe = av_frame_alloc();
    aframe = av_frame_alloc();
    packet = av_packet_alloc();
    swidth = vidpar->width;
    sheight = vidpar->height;
    screen = SDL_CreateWindow("Fplay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        swidth, sheight, SDL_WINDOW_OPENGL);
    if (!screen) {
        perror("screen");
        goto clean_packet_frame;
    }
    renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        perror("renderer");
        goto clean_renderer;
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING | SDL_TEXTUREACCESS_TARGET,
        swidth, sheight);
    if (!texture) {
        perror("texture");
        goto clean_texture;
    }
    rect.x = 0;
    rect.y = 0;
    rect.w = swidth;
    rect.h = sheight;

    SDL_zero(want);
    SDL_zero(have);
    want.samples = audpar->sample_rate;
    want.channels = audpar->channels;
    auddev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    SDL_PauseAudioDevice(auddev, 0);
    if (!auddev) {
        perror("auddev");
        goto clean_audio_device;
    }
    SDL_Event evt;
    windowID = SDL_GetWindowID(screen);
    while (running) {
        while (av_read_frame(pFormatCtx, packet) >= 0)
        {
            while (SDL_PollEvent(&evt))
            {
                switch (evt.type) {
                    case SDL_WINDOWEVENT: {
                        if (evt.window.windowID == windowID) {
                            switch (evt.window.event) {
                                case SDL_WINDOWEVENT_CLOSE: {
                                    evt.type = SDL_QUIT;
                                    running = false;
                                    SDL_PushEvent(&evt);
                                    break;
                                }
                            };
                        }
                        break;
                    }
                    case SDL_QUIT: {
                        running = false;
                        break;
                    }
                }
            }
            if (packet->stream_index == vidId) {
                display(vidCtx, packet, vframe, &rect,
                    texture, renderer, fpsrendering);

            } else if (packet->stream_index == audId) {
                // playaudio(audCtx, packet, aframe, auddev);

            }
            av_packet_unref(packet);
        }
    }


    clean_audio_device:
      SDL_CloseAudioDevice(auddev);
    clean_texture:
      SDL_DestroyTexture(texture);
    clean_renderer:
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(screen);
    clean_packet_frame:
      av_packet_free(&packet);
      av_frame_free(&vframe);
      av_frame_free(&aframe);
    clean_codec_context:
      avcodec_free_context(&vidCtx);
      avcodec_free_context(&audCtx);
    clean_format_context:
      avformat_close_input(&pFormatCtx);
      avformat_free_context(pFormatCtx);

    SDL_Quit();

    return 0;
}


void display(AVCodecContext* ctx, AVPacket* pkt, AVFrame* frame, SDL_Rect* rect,
    SDL_Texture* texture, SDL_Renderer* renderer, double fpsrend)
{
    time_t start = time(NULL);
    if (avcodec_send_packet(ctx, pkt) < 0) {
        perror("send packet");
        return;
    }
    if (avcodec_receive_frame(ctx, frame) < 0) {
        perror("receive frame");
        return;
    }
    int framenum = ctx->frame_number;
    if ((framenum % 1000) == 0) {
        printf("Frame %d (size=%d pts %d dts %d key_frame %d"
            " [ codec_picture_number %d, display_picture_number %d\n",
            framenum, frame->pkt_size, frame->pts, frame->pkt_dts, frame->key_frame,
            frame->coded_picture_number, frame->display_picture_number);
    }
    SDL_UpdateYUVTexture(texture, rect,
        frame->data[0], frame->linesize[0],
        frame->data[1], frame->linesize[1],
        frame->data[2], frame->linesize[2]);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, rect);
    SDL_RenderPresent(renderer);
    time_t end = time(NULL);
    double diffms = difftime(end, start) / 1000.0;
    if (diffms < fpsrend) {
        uint32_t diff = (uint32_t)((fpsrend - diffms) * 1000);
        printf("diffms: %f, delay time %d ms.\n", diffms, diff);
        SDL_Delay(diff);
    }
}

void playaudio(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame,
    SDL_AudioDeviceID auddev)
{
    // if (avcodec_send_packet(ctx, pkt) < 0) {
    //     perror("send packet");
    //     return;
    // }
    // if (avcodec_receive_frame(ctx, frame) < 0) {
    //     perror("receive frame");
    //     return;
    // }
    // int size;
    // int bufsize = av_samples_get_buffer_size(&size, ctx->channels,
    //         frame->nb_samples, frame->format, 0);
    // bool isplanar = av_sample_fmt_is_planar(frame->format) == 1;
    // for (int ch = 0; ch < ctx->channels; ch++) {
    //     if (!isplanar) {
    //         if (SDL_QueueAudio(auddev, frame->data[ch], frame->linesize[ch]) < 0) {
    //             perror("playaudio");
    //             printf(" %s\n", SDL_GetError());
    //             return;
    //         }
    //     } else {
    //         if (SDL_QueueAudio(auddev, frame->data[0] + size*ch, size) < 0) {
    //             perror("playaudio");
    //             printf(" %s\n", SDL_GetError());
    //             return;
    //         }
    //     }
    // }
}
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
    
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "SDL.h"
};


//Refresh Event
#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

int thread_exit = 0;
void display(AVCodecContext*, AVPacket*, AVFrame*, SDL_Rect*, SDL_Texture*, SDL_Renderer*, double);

void playaudio(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame, SDL_AudioDeviceID auddev);

int sfp_refresh_thread(void* opaque) {
    
	thread_exit = 0;
	while (!thread_exit) {
    
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	thread_exit = 0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}




int main(int argc, char* argv[])
{
    
	AVFormatContext* pFormatCtx;
	int				i;
  AVCodecContext *vidCtx, *audCtx;
  AVCodec *vidCodec, *audCodec;
	AVFrame* vFrame,* aFrame, * pFrameYUV;
	uint8_t* out_buffer;
	AVPacket* packet;
	int ret, got_picture;
  double fpsrendering = 0.0;
  int vidId = -1, audId = -1;
  AVCodecParameters *vidpar, *audpar;

	//------------SDL----------------
	int screen_w, screen_h;
	SDL_Window* screen;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Rect sdlRect;
	SDL_Thread* video_tid;
	SDL_Event event;
  SDL_AudioDeviceID auddev;
  SDL_AudioSpec want, have;

	struct SwsContext* img_convert_ctx;

	char filepath[] = "../sample_960x400_ocean_with_audio.ts";


	//av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
    
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    
		printf("Couldn't find stream information.\n");
		return -1;
	}
	vidId = -1;

	// for (i = 0; i < pFormatCtx->nb_streams; i++)
		// if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    //     AVRational rational = pFormatCtx->streams[i]->avg_frame_rate;
    //     fpsrendering = 1.0 / ((double)rational.num / (double)(rational.den));
		// 	videoindex = i;
		// 	break;
		// }
   bool foundVideo = false, foundAudio = false;
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
          	printf("Found audio.\n");
            audCodec = localcodec;
            audpar = localparam;
            audId = i;
            foundAudio = true;
        }
        if (foundVideo && foundAudio) { break; }
    }
	if (vidId == -1) {
    
		printf("Didn't find a video stream.\n");
		return -1;
	}

	AVCodecParameters* codecParameters = pFormatCtx->streams[vidId]->codecpar;
	vidCtx = avcodec_alloc_context3(vidCodec);
  audCtx = avcodec_alloc_context3(audCodec);
	avcodec_parameters_to_context(vidCtx, vidpar);
	avcodec_parameters_to_context(vidCtx, audpar);

	//pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	  if (avcodec_parameters_to_context(vidCtx, vidpar) < 0) {
        perror("vidCtx");
    }
    if (avcodec_parameters_to_context(audCtx, audpar) < 0) {
        perror("audCtx");
    }
    if (avcodec_open2(vidCtx, vidCodec, NULL) < 0) {
        perror("vidCtx");
    }
    if (avcodec_open2(audCtx, audCodec, NULL) < 0) {
        perror("audCtx");
    }

	packet = av_packet_alloc();
	vFrame = av_frame_alloc();
	aFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	out_buffer = (uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, vidCtx->width, vidCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, vidCtx->width, vidCtx->height, 1);

	img_convert_ctx = sws_getContext(vidCtx->width, vidCtx->height, vidCtx->pix_fmt,
		vidCtx->width, vidCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    SwrContext *resampler = swr_alloc_set_opts(NULL, 
                                           audCtx->channel_layout,
                                           AV_SAMPLE_FMT_S16,
                                           44100,
                                           audCtx->channel_layout,
                                           audCtx->sample_fmt,
                                           audCtx->sample_rate,
                                           0, 
                                           NULL);


        swr_init(resampler);


	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO)) {
    
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	//SDL 2.0 Support for multiple windows
	screen_w = vidCtx->width;
	screen_h = vidCtx->height;
	screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL);

	if (!screen) {
    
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	//IYUV: Y + U + V (3 planes)
	//YV12: Y + V + U (3 planes)
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, vidCtx->width, vidCtx->height);

	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = screen_w;
	sdlRect.h = screen_h;

   SDL_zero(want);
    SDL_zero(have);
    want.samples = audpar->sample_rate;
    want.channels = audpar->channels;
    want.format = AUDIO_S16SYS;
    want.freq = 44100;

    auddev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    SDL_PauseAudioDevice(auddev, 0);
    if (!auddev) {
        perror("auddev");
    }

	packet = (AVPacket*)av_malloc(sizeof(AVPacket));

	video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);
	//------------SDL End------------
	//Event Loop
  bool running = true;
   SDL_Event evt;
	while (running) {
        while (av_read_frame(pFormatCtx, packet) >= 0)
        {
            while (SDL_PollEvent(&evt))
            {
                switch (evt.type) {
                    case SDL_WINDOWEVENT: {
                     
                            switch (evt.window.event) {
                                case SDL_WINDOWEVENT_CLOSE: {
                                    evt.type = SDL_QUIT;
                                    running = false;
                                    SDL_PushEvent(&evt);
                                    thread_exit = 1;
                                    break;
                                }
                            };
                        
                        break;
                    }
                    case SDL_QUIT: {
                        running = false;
                        break;
                    }
                }
            }
            if (packet->stream_index == vidId) {
                display(vidCtx, packet, vFrame, &sdlRect,
                    sdlTexture, sdlRenderer, fpsrendering);

            }  else if (packet->stream_index == audId) {
          

        ret = avcodec_send_packet(audCtx, packet);
    
            ret = avcodec_receive_frame(audCtx, aFrame);
            if (ret >= 0){
                int dst_samples = aFrame->channels * av_rescale_rnd(swr_get_delay(resampler, aFrame->sample_rate) + aFrame->nb_samples, 44100, aFrame->sample_rate, AV_ROUND_UP);
                uint8_t *audiobuf = NULL;
                ret = av_samples_alloc(&audiobuf, NULL, 1, 
                                       dst_samples,
                                       AV_SAMPLE_FMT_S16, 
                                       1);
                dst_samples = aFrame->channels * swr_convert(
                                                 resampler, 
                                                 &audiobuf, 
                                                 dst_samples,
                                                 (const uint8_t**) aFrame->data, 
                                                 aFrame->nb_samples);
                ret = av_samples_fill_arrays(aFrame->data, 
                                             aFrame->linesize, 
                                             audiobuf,
                                             1, 
                                             dst_samples, 
                                             AV_SAMPLE_FMT_S16, 
                                             1);
                SDL_QueueAudio(auddev, 
                               aFrame->data[0], 
                               aFrame->linesize[0]); 
            }

        



                // playaudio(audCtx, packet, aFrame, auddev);

            }
            av_packet_unref(packet);
        }
    }

	sws_freeContext(img_convert_ctx);

	SDL_Quit();
	//--------------
	av_frame_free(&pFrameYUV);
	av_frame_free(&vFrame);
	av_frame_free(&aFrame);
	avcodec_close(vidCtx);
	avformat_close_input(&pFormatCtx);

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
        printf("Frame %d (size=%d pts %lld dts %lld key_frame %d"
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

void playaudio(AVCodecContext *ctx, AVPacket *pkt, AVFrame *frame, SDL_AudioDeviceID auddev)
{
    if (avcodec_send_packet(ctx, pkt) < 0) {
        perror("send packet");
        return;
    }
    if (avcodec_receive_frame(ctx, frame) < 0) {
        perror("receive frame");
        return;
    }
    int size;
    // int bufsize = av_samples_get_buffer_size(&size, ctx->channels,
    //         frame->nb_samples, frame->format, 0);
     bool isplanar = true;
     SDL_QueueAudio(auddev, frame->data[0], frame->linesize[0]);
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
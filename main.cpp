#include <stdlib.h>
#include <stdio.h>
#include <iostream>

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define MAX_AUDIO_FRAME_SIZE 119200

using namespace std;

static void decode(AVCodecContext *codecContext, AVPacket *packet, AVFrame *frame, FILE *outFile)
{
    int ret, data_size;

    //send the packet with the compressed data to the decoder
    ret = avcodec_send_packet(codecContext, packet);
    if(ret < 0)
    {
        cout << "Error submitting the packet to the decoder" << endl;
        exit(1);
    }

    //read all the output frames (in general there may be any number of them)
    while(ret >= 0)
    {
        ret = avcodec_receive_frame(codecContext, frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return;
        }else if(ret < 0)
        {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        data_size = av_get_bytes_per_sample(codecContext->sample_fmt);
        if(data_size < 0)
        {
            //This should not occur, checking just for paranoia
            fprintf(stderr, "Failed to calciulate data size");
            exit(1);
        }
        for(int i = 0; i < frame->nb_samples; i++)
        {
            for(int ch = 0; ch < codecContext->channels; ch++)
            {
                fwrite(frame->data[ch] + data_size * i, 1, data_size, outFile);
            }
        }
    }
}

int main()
{

    avcodec_register_all();
    const char *inputFileName = "./fumika - Endless Road.mp3";
    const char *outputFileName = "./Endless Road.pcm";
    const AVCodec *codec;
    AVCodecContext *codecContext = NULL;
    AVCodecParserContext *parserContext = NULL;
    AVFormatContext *formatContext = NULL;
    AVPacket *packet;
    AVFrame *frame = NULL;

    int len, ret;
    FILE *in, *out;

    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;

    // in = fopen(inputFileName, "rb");
    // if(!in)
    // {
        
    //     fprintf(stderr, "can not open media file %s\n", inputFileName);
    //     exit(1);
    // }

    out = fopen(outputFileName, "wb");
    if(!out)
    {
        av_free(codecContext);
        fprintf(stderr, "can not create file %s \n", outputFileName);
        exit(1);
    }

    formatContext = avformat_alloc_context();
    if(avformat_open_input(&formatContext, inputFileName, NULL, NULL) != 0)
    {
        fprintf(stderr, "Couldn't open input stream\n");
        exit(1);
    }

    if(avformat_find_stream_info(formatContext, NULL) < 0)
    {
        fprintf(stderr, "Couldn't find stream infomation\n");
        exit(1);
    }

    av_dump_format(formatContext, 0, inputFileName, false);


    int audioStream = -1;
    for(int i = 0; i < formatContext->nb_streams; i++)
    {
        if(formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStream = i;
            break;
        }
    }

    if(audioStream == -1)
    {
        fprintf(stderr, "Couldn't find audio stream\n");
        exit(1);
    }

    codecContext = formatContext->streams[audioStream]->codec;

    if(!codecContext)
    {
        
        fprintf(stderr, "can not alloc codec context\n");
        exit(1);
    }

    

    //find the MPEG audio decoder
    codec = avcodec_find_decoder(codecContext->codec_id);
    if(!codec)
    {
        
        fprintf(stderr, "codec not found\n");
        exit(1);
    }


   

    
    //open codec
    if(avcodec_open2(codecContext, codec, NULL) < 0)
    {
        
        fprintf(stderr, "can not open codec\n");
        exit(1);
    }

    int32_t in_sample_rate = codecContext->sample_rate;
    int32_t in_channels = codecContext->channels;
    AVSampleFormat in_sample_fmt = codecContext->sample_fmt;
    uint64_t in_channel_layout = av_get_default_channel_layout(codecContext->channels);

    cout << "sample rate: " << in_sample_fmt << endl;
    cout << "channels: " << in_channels << endl;
    cout << "sample fromat: " << in_sample_fmt << endl;

    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int out_nb_samples = codecContext->frame_size;
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);
    cout << "out buffer size: " << out_buffer_size << endl;

    uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

    packet = av_packet_alloc();
    av_init_packet(packet);
    frame = av_frame_alloc();
    int gotPicture;


    struct SwrContext *au_convert_ctx;
    
    au_convert_ctx = swr_alloc();
    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate, in_channel_layout, codecContext->sample_fmt, codecContext->sample_rate, 0, NULL);
    swr_init(au_convert_ctx);


    while(av_read_frame(formatContext, packet) == 0)
    {
        if(packet->size > 0 && packet->stream_index == audioStream)
        {
            ret = avcodec_decode_audio4(codecContext, frame, &gotPicture, packet);
            if(ret < 0)
            {
                fprintf(stderr, "Error in decoding audio frame\n");
                exit(1);
            }
            if(gotPicture > 0)
            {
                swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t**)frame->data, frame->nb_samples);
                printf("pts: %lld\t packet size: %d\n", index, packet->pts, packet->size);
                printf("frame samples: %d\n", frame->nb_samples);
                fwrite(out_buffer, sizeof(uint8_t), out_buffer_size, out);
                
            }
        }
        av_free_packet(packet);
    }
    

    swr_free(&au_convert_ctx);
    fclose(out);
    av_free(out_buffer);
    
    avcodec_close(codecContext);

    avformat_close_input(&formatContext);


    return 0;
}
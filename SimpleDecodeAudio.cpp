#include "SimpleDecodeAudio.h"
#include <string>
#include <map>
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

void SimpleDecodeAudio::decodeAudio(const char* inputFileName, const char* outputFileName)
{
    avcodec_register_all();
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

    av_format_inject_global_side_data(formatContext);

    if(avformat_find_stream_info(formatContext, NULL) < 0)
    {
        fprintf(stderr, "Couldn't find stream information\n");
        exit(1);
    }


	AVDictionaryEntry *tag = NULL;
	// map<string, string> infoMap;
	cout << "media info:" << endl;
	while(tag = av_dict_get(formatContext->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))
	{
		string keyStr = tag->key;
		string valueStr = tag->value;
		// infoMap.insert(keyStr, valueStr);
		cout << keyStr << ": " << valueStr << endl;
	}

    for(int i = 0; i < formatContext->nb_streams; i++)
	{
		if(formatContext->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC)
		{
			printf("pic index: %d\n", i);
			AVPacket p = formatContext->streams[i]->attached_pic;
			FILE *picFile = fopen("./picture.jpeg", "wb");
			fwrite(p.data, sizeof(uint8_t), p.size, picFile);
			fflush(picFile);
			fclose(picFile);
			picFile = NULL;
			break;

		}
	}


    int audioStream = -1;
    for(int i = 0; i < formatContext->nb_streams; i++)
    {
        if(formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStream = i;
            break;
        }
    }

	printf("Audio stream index = %d\n", audioStream);

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

    packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    frame = av_frame_alloc();

    int gotPicture;


    struct SwrContext *au_convert_ctx;
    
    au_convert_ctx = swr_alloc();
    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate, in_channel_layout, codecContext->sample_fmt, codecContext->sample_rate, 0, NULL);
    swr_init(au_convert_ctx);

	
	int currentSkip = 0;

    int64_t stream_start_time = formatContext->streams[audioStream]->start_time;
    int64_t pkt_ts = 0;

    while(av_read_frame(formatContext, packet) >= 0)
    {
		// printf("Packet stream index = %d\n", packet->stream_index);
        if(packet->stream_index == audioStream)
        {
			pkt_ts = packet->pts == AV_NOPTS_VALUE ? packet->dts : packet->pts;
            if(pkt_ts < stream_start_time)
            {
                continue;
            }
			if(avcodec_send_packet(codecContext, packet))
			{
				continue;
			}

			if(avcodec_receive_frame(codecContext, frame))
			{
				continue;
			}

			swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t**)frame->data, frame->nb_samples);
			// printf("frame samples: %d\n", frame->nb_samples);
			fwrite(out_buffer, sizeof(uint8_t), out_buffer_size, out);

            
        }
        av_packet_unref(packet);
        
    }
    
    av_free_packet(packet);
    swr_free(&au_convert_ctx);
    fclose(out);
    av_free(out_buffer);
    
    avcodec_close(codecContext);

    avformat_close_input(&formatContext);
}
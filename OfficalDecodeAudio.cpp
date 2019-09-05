#include "OfficalDecodeAudio.h"

extern "C"
{
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
}

void OfficalDecodeAudio::decodeAudio(const char *inputFileName, const char *outputFileName)
{
    int err = 0;
    AVFormatContext *formatContext = NULL;
    AVInputFormat *inputFormat = av_find_input_format("mp3");

    if(!inputFormat)
    {
        fprintf(stderr, "Can't detect the format of input file\n");
        // exit(1);
    }

    formatContext = avformat_alloc_context();
    if(!formatContext)
    {
        fprintf(stderr, "Can't allocate context\n");
        exit(1);
    }

    err = avformat_open_input(&formatContext, inputFileName, NULL, NULL);
    if(err < 0)
    {
        fprintf(stderr, "Can't open input file\n");
        exit(1);
    }

    av_format_inject_global_side_data(formatContext);

    av_dump_format(formatContext, 0, inputFileName, 0);

    int audioIndex = -1;

    for(int i = 0; i < formatContext->nb_streams; i++)
    {
        AVStream *st = formatContext->streams[i];
        AVMediaType type = st->codecpar->codec_type;
        if(type == AVMEDIA_TYPE_AUDIO)
        {
            audioIndex = i;
        }
    }

    if(audioIndex == -1)
    {
        fprintf(stderr, "Can't find audio stream\n");
        exit(1);
    }

    printf("audio index is %d\n", audioIndex);

    audioIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, audioIndex, -1, NULL, 0);

    if(audioIndex < 0)
    {
        fprintf(stderr, "Can't find best audio stream\n");
        exit(1);
    }

    AVCodecContext *codecCtx;
    AVCodec *codec;

    int32_t in_sample_rate;
    int32_t in_channels;
    AVSampleFormat in_sample_fmt;
    uint64_t in_channel_layout;

    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int32_t out_sample_rate = 44100;
    int32_t out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    
    codecCtx = avcodec_alloc_context3(NULL);
    if(!codecCtx)
    {
        fprintf(stderr, "Error when alloc CodecContext\n");
        exit(1);
    }

    err = avcodec_parameters_to_context(codecCtx, formatContext->streams[audioIndex]->codecpar);

    if(err < 0)
    {
        fprintf(stderr, "Failed set params to CodecContext\n");
        exit(1);
    }

    codecCtx->pkt_timebase = formatContext->streams[audioIndex]->time_base;

    codec = avcodec_find_decoder(codecCtx->codec_id);

    codecCtx->codec_id = codec->id;
    codecCtx->flags2 |= AV_CODEC_FLAG2_FAST;

    formatContext->streams[audioIndex]->discard = AVDISCARD_DEFAULT;

    in_sample_rate = codecCtx->sample_rate;
    in_channels = codecCtx->channels;
    in_channel_layout = codecCtx->channel_layout;

    

    


}        
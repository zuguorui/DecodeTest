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
    AVFormatContext *formatCtx = NULL;
    

    formatCtx = avformat_alloc_context();
    if(!formatCtx)
    {
        fprintf(stderr, "Can't allocate context\n");
        exit(1);
    }

    err = avformat_open_input(&formatCtx, inputFileName, NULL, NULL);
    if(err < 0)
    {
        fprintf(stderr, "Can't open input file\n");
        exit(1);
    }

    if(avformat_find_stream_info(formatCtx, NULL) < 0)
    {
        fprintf(stderr, "Error when find stream info\n");
        exit(1);
    }

    av_format_inject_global_side_data(formatCtx);

    av_dump_format(formatCtx, 0, inputFileName, 0);

    int audioIndex = -1;

    for(int i = 0; i < formatCtx->nb_streams; i++)
    {
        AVStream *st = formatCtx->streams[i];
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
    
    codecCtx = formatCtx->streams[audioIndex]->codec;
    if(!codecCtx)
    {
        fprintf(stderr, "Error when alloc CodecContext\n");
        exit(1);
    }



    codec = avcodec_find_decoder(codecCtx->codec_id);

    if(avcodec_open2(codecCtx, codec, NULL) < 0)
    {
        fprintf(stderr, "Error when open codec\n");
        exit(1);
    }

    formatCtx->streams[audioIndex]->discard = AVDISCARD_DEFAULT;

    in_sample_rate = codecCtx->sample_rate;
    in_channels = codecCtx->channels;
    in_channel_layout = codecCtx->channel_layout;

    

    


}        
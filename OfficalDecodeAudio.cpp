#include "OfficalDecodeAudio.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

#define MAX_FRAME_SIZE 4096

enum DecodeState
{
    NORMAL,
    NEED_READ_FIRST,
    NEED_NEW_PACKET
};

void OfficalDecodeAudio::decodeAudio(const char *inputFileName, const char *outputFileName)
{

    FILE *out;
    out = fopen(outputFileName, "wb");
    if (!out)
    {
        printf("can not create file %s \n", outputFileName);
        exit(1);
    }

    av_register_all();

    int err = 0;
    AVFormatContext *formatCtx = NULL;

    formatCtx = avformat_alloc_context();
    if (!formatCtx)
    {
        printf("Can't allocate context\n");
        exit(1);
    }

    err = avformat_open_input(&formatCtx, inputFileName, NULL, NULL);
    if (err < 0)
    {
        printf("Can't open input file\n");
        exit(1);
    }

    if (avformat_find_stream_info(formatCtx, NULL) < 0)
    {
        printf("Error when find stream info\n");
        exit(1);
    }

    av_dump_format(formatCtx, 0, inputFileName, 0);

    int audioIndex = -1;

    for (int i = 0; i < formatCtx->nb_streams; i++)
    {
        AVStream *st = formatCtx->streams[i];
        AVMediaType type = st->codecpar->codec_type;
        if (type == AVMEDIA_TYPE_AUDIO)
        {
            audioIndex = i;
        }
        if (st->disposition & AV_DISPOSITION_ATTACHED_PIC)
        {
            printf("pic index: %d\n", i);
            AVPacket p = st->attached_pic;
            FILE *picFile = fopen("./picture.jpeg", "wb");
            fwrite(p.data, sizeof(uint8_t), p.size, picFile);
            fflush(picFile);
            fclose(picFile);
            picFile = NULL;
        }
    }

    if (audioIndex == -1)
    {
        printf("Can't find audio stream\n");
        exit(1);
    }

    printf("audio index is %d\n", audioIndex);

    AVStream *audioStream = formatCtx->streams[audioIndex];

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

    codec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (codec == NULL)
    {
        printf("can not find codec");
        exit(1);
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx)
    {
        printf("Error when alloc CodecContext\n");
        exit(1);
    }

    if (avcodec_parameters_to_context(codecCtx, audioStream->codecpar) < 0)
    {
        printf("Error when copy params to codec context");
        exit(1);
    }
    if (avcodec_open2(codecCtx, codec, NULL) < 0)
    {
        fprintf(stderr, "Error when open codec\n");
        exit(1);
    }

    in_sample_rate = codecCtx->sample_rate;
    in_channels = codecCtx->channels;
    in_channel_layout = codecCtx->channel_layout;
    in_sample_fmt = codecCtx->sample_fmt;

    printf("Convert data format:\n");
    printf("    in_sample_rate: %d\n", in_sample_rate);
    printf("    in_sample_fmt: %d\n", in_sample_fmt);
    printf("    in_channels: %d\n", in_channels);
    printf("    in_channel_layout: %ld\n", in_channel_layout);

    printf("    out_sample_rate: %d\n", out_sample_rate);
    printf("    out_sample_fmt: %d\n", out_sample_fmt);
    printf("    out_channels: %d\n", out_channels);
    printf("    out_channel_layout: %ld\n", out_channel_layout);

    SwrContext *convert_context;
    convert_context = swr_alloc();
    convert_context = swr_alloc_set_opts(convert_context, out_channel_layout, out_sample_fmt, out_sample_rate, in_channel_layout, in_sample_fmt, in_sample_rate, 0, NULL);

    swr_init(convert_context);

    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    AVFrame *frame = av_frame_alloc();

    int64_t stream_start_time = audioStream->start_time;
    int64_t pkt_pts;

    DecodeState decode_state = NORMAL;

    // int32_t out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, MAX_FRAME_SIZE, out_sample_fmt, 1);
    int32_t out_buffer_size = MAX_FRAME_SIZE * 2 * sizeof(int16_t);
    printf("out_buffer_size = %d\n", out_buffer_size);
    uint8_t *out_buffer = (uint8_t *)malloc(out_buffer_size);

    bool readFinished = false;
    while (1)
    {

        if (decode_state != NEED_READ_FIRST)
        {
            int err1 = av_read_frame(formatCtx, packet);
            if (err1 < 0)
            {
                readFinished = true;
                packet->size = 0;
                printf("av_read_frame returns %d\n", err1);
            }
        }

        // printf("presention time = %ld\n", pkt_pts);
        int err2 = avcodec_send_packet(codecCtx, packet);
        if (err2 == AVERROR(EAGAIN))
        {
            //Need to call avcodec_receive_frame() first, and then resend
            //this packet.
            decode_state = NEED_READ_FIRST;
            printf("call send_packet() returns AVERROR(EAGAIN)\n");
        }
        else if (err2 == AVERROR_EOF)
        {
            printf("call send_packet() returns AVERROR_EOF\n");
        }
        else if (err2 == AVERROR(EINVAL))
        {
            printf("call send_packet() returns AVERROR(EINVAL)\n");
        }
        else if (err2 < 0)
        {
            printf("call send_packet() returns %d\n", err2);
        }
        else
        {
            printf("call send_packet() normal, returns %d\n", err2);
            decode_state = NORMAL;
        }

        while (1)
        {
            int err3 = avcodec_receive_frame(codecCtx, frame);
            if (err3 == AVERROR(EAGAIN))
            {
                //Can not read until send a new packet

                printf("call avcodec_receive_frame() returns AVERROR(EAGAIN)\n");
                break;
            }
            else if (err3 == AVERROR_EOF)
            {
                printf("call avcodec_receive_frame() returns AVERROR_EOF\n");
                break;
            }
            else if (err3 == AVERROR(EINVAL))
            {
                printf("call avcodec_receive_frame() returns AVERROR(EINVAL)\n");
                break;
            }
            else if (err < 0)
            {
                printf("call avcodec_receive_frame() returns %d\n", err3);
                break;
            }
            else
            {
                printf("success receive a frame\n");
                int frameCount = swr_convert(convert_context, &out_buffer, MAX_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
                fwrite(out_buffer, frameCount, out_channels * 2 * sizeof(uint8_t), out);
                while (1)
                {
                    frameCount = swr_convert(convert_context, &out_buffer, MAX_FRAME_SIZE, NULL, 0);
                    if (frameCount > 0)
                    {
                        fwrite(out_buffer, frameCount, out_channels * 2 * sizeof(uint8_t), out);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        //If decode_state is NEED_READ_FIRST, we need to call avcodec_receive_frame() first, and then resend this packet to avcodec_send_packet, so we can't unref it and
        // can't install new data by av_read_frame().
        if (decode_state != NEED_READ_FIRST)
        {
            av_packet_unref(packet);
        }

        if (readFinished)
        {
            break;
        }
    }

    fflush(out);
    fclose(out);
    av_frame_free(&frame);
    av_free_packet(packet);
    swr_free(&convert_context);
    avcodec_close(codecCtx);
    avformat_close_input(&formatCtx);
}
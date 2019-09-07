# 1 关键结构体

## 1.1 VideoState

包含解码器以及状态信息等的结构体

```C
typedef struct VideoState {
    SDL_Thread *read_tid;
    AVInputFormat *iformat;
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;
    int queue_attachments_req;
    int seek_req;
    int seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    AVFormatContext *ic;
    int realtime;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    FrameQueue subpq;
    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;
    Decoder subdec;

    int audio_stream;

    int av_sync_type;

    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    int muted;
    struct AudioParams audio_src;
#if CONFIG_AVFILTER
    struct AudioParams audio_filter_src;
#endif
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    int frame_drops_early;
    int frame_drops_late;

    enum ShowMode {
        SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
    } show_mode;
    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;
    int last_i_start;
    RDFTContext *rdft;
    int rdft_bits;
    FFTSample *rdft_data;
    int xpos;
    double last_vis_time;
    SDL_Texture *vis_texture;
    SDL_Texture *sub_texture;
    SDL_Texture *vid_texture;

    int subtitle_stream;
    AVStream *subtitle_st;
    PacketQueue subtitleq;

    double frame_timer;
    double frame_last_returned_time;
    double frame_last_filter_delay;
    int video_stream;
    AVStream *video_st;
    PacketQueue videoq;
    double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    struct SwsContext *img_convert_ctx;
    struct SwsContext *sub_convert_ctx;
    int eof;

    char *filename;
    int width, height, xleft, ytop;
    int step;

#if CONFIG_AVFILTER
    int vfilter_idx;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
    AVFilterGraph *agraph;              // audio filter graph
#endif

    int last_video_stream, last_audio_stream, last_subtitle_stream;

    SDL_cond *continue_read_thread;
} VideoState;
```

## 1.2 MyAVPacketList

一个AVPacket的单向列表的节点

```C
typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList *next;
    int serial;
} MyAVPacketList;
```

## 1.3 PacketQueue

一个AVPacket的单向列表

```C
typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;//头节点和尾节点
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;
```

## 1.4 AudioParams

定义了一些音频参数

```C
typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;
```


## 1.5 Clock

尚不知道有什么用

```C
typedef struct Clock {
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;
```

## 1.6 Frame

定义一个包含AVFrame的结构体，包含一些必要参数以确定AVFrame的状态

```C
/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;          /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;
```

## 1.7 FrameQueue

定义一个Frame的队列，其中包含了Frame数组。

```C
typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    SDL_mutex *mutex;
    SDL_cond *cond;
    PacketQueue *pktq;
} FrameQueue;
```

## 1.8 Decoder

定义一个解码器的结构体，包含AVCodecContext、AVPacket以及AVPacket的列表。

```C
typedef struct Decoder {
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    SDL_cond *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    SDL_Thread *decoder_tid;
} Decoder;
```

# 2 流程及关键函数分析

## 2.1 main

1. 通过`parse_options()`解析输入参数。
2. 初始化`flush_pkt`，该变量为一个AVPacket类型。
通过`stream_open(input_filename, file_iformat)`方法获取一个`ViewState`类型的变量is，is就作为一个持有输入文件、参数以及编解码的结构体在之后的流程中被使用。`file_format`是从输入参数中解析出来的，其命令为`f`，具体函数是`opt_format`，该函数最终调用了`avformat.h`中的`AVInputFormat *av_find_input_format(const char *short_name)`函数，具体猜测可能是通过mp3、mp4这样的字符串来寻找具体的格式信息。但这一步并不是一定会成功的，因为多数情况下我们基本不会输入`-f`参数也可以正常播放。
3. 最后通过`event_loop(is)`函数开始播放循环。

## 2.2 stream_open
作用：打开一个输入流，并初始化解码器参数。

完整声明：
```C
static VideoState *stream_open(const char *filename, AVInputFormat *iformat)
```
1. 新建VideoState *is并初始化，设置filename、输入文件格式iformat、左上角坐标。
2. 调用`frame_queue_init()`函数，分别初始化is的video、audio和subtitle的frame队列和packet队列，注意在这个函数中，会将video的PacketQueue赋值给FrameQueue的pktq变量，audio和subtitle也是同理。然后初始化FrameQueue中的所有AVFrame。
3. 调用`packet_queue_init()`，分别初始化is的video、audio和subtitle的PacketQueue。
4. 设置is的锁、clock。
5. 新建一个SDL线程，设置给is->read_tid，其回调函数是`read_thread`函数

## 2.3 `event_loop`

接受SDL的操作事件，并持续刷新输出。该函数主要是针对各种按键输入进行处理，现在假设我们并没有按任何按键，它就只有调用`refresh_loop_wait_event(cur_stream, &event)`这一步了，而cur_stream就是我们的VideoState is了。

完整声明：
```C
static void event_loop(VideoState *cur_stream)
```

## 2.4 `refresh_loop_wait_event`

检测事件，同时管理光标何时隐藏和显示（在ffplay的窗口中光标过一会儿是会自动隐藏的）。主要是通过调用`void video_refresh(void *opaque, double *remaining_time)`来进行刷新。

完整声明：
```C
static void refresh_loop_wait_event(VideoState *is, SDL_Event *event)
```

## 2.5 `video_refresh`

完整声明：
```C
static void video_refresh(void *opaque, double *remaining_time)
```
opaque传入的是VideoState is。

1. 检查播放状态，这些状态就存储在is中
2. 如果is的audio stream不为空，且当前显示模式不是video，并且允许display，那么会调用`video_display(is)`。
3. 如果is的video stream不为空，那么就处理视频数据。并且在这种情况下，如果subtitle不为空，还要处理subtitle。
4. 最终调用`video_display(is)`进行显示。

## 2.6 `video_display`

完整声明：
```C
/* display the current picture, if any */
static void video_display(VideoState *is)
```

1. 检查是否打开了一个窗口用以显示（检查is->width是否有值，如果不为0说明窗口已经打开了）。如果没有，通过`video_open(is)`打开一个窗口，并将窗口的width和height分别赋值给is->width和is->height。
2. 设置一些SDL参数。
3. 如果is的audio stream不为空且显示模式不为SHOW_MODE_VIDEO，那么说明应该播放的是音频，调用`video_audio_display(is)`显示。
4. 如果is的video stream不为空，说明应该播放视频，调用`video_image_display(is)`显示。

## 2.7 `video_audio_display`

完整声明：
```C
static void video_audio_display(VideoState *s)
```
用来在视频上显示当音频正在播放时候的内容，这里和is->show_mode有关，在`OptionDef options[]`中查找`opt_show_mode`，就可以看到对应的解释：
**{ "showmode", HAS_ARG, { .func_arg = opt_show_mode}, "select show mode (0 = video, 1 = waves, 2 = RDFT)", "mode" }**
show_mode在VideoState中定义的。
```C
enum ShowMode {
    SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
} show_mode
```
但从命令行仅接受0～2这三个数，在使用ffplay播放音频时，输入格式如下：
`ffplay inputfile -showmode 0`
0：如果音频文件中有专辑图片的话，就会显示专辑图片，mp3文件中一般会有。如果没有就不显示，也不会开窗口。
1: 显示波形图
2: 显示频谱图

## 2.8 `video_image_display`

完整声明：
```C
static void video_image_display(VideoState *is)
```
显示视频以及字幕。对于音频文件来说，这个函数仅会显示专辑图片，但对于视频文件来说，它是真正实现播放的地方。

从is中获取视频以及字幕的AVFrame，并使用SDL播放。

## 2.9 `read_thread`

完整声明：
```C
static int read_thread(void *arg)
```

该函数在一个单独的线程中被调用，该函数专门负责打开输入、解码。

1. 新建一堆变量，需要注意的是一个数组`int st_index[AVMEDIA_TYPE_NB]`，它用来存储各种媒体类型，AVMEDIA_TYPE_NB = 5，一共5种类型。
2. 新建一个AVFormatContext ic。
3. 检查format_opts，该变量是由cmdutils.c在解析输入命令的时候初始化的，但解析过程太过繁琐。如果format_opts中键`scan_all_pmts`没有值，那么就为它设置一个值：
```C
av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE)
```
并且将`scan_all_pmts_set = 1`。
所以干脆我们就直接这么设置。
4. 调用
```C
avformat_open_input(&ic, is->filename, is->iformat, &format_opts)
```
打开输入，之前说过is->iformat是由输入命令指定的，如果在输入时未指定，那它就是空的。然后将ic赋值给is->ic。
如果之前手动设置了format_opts，那么这时要清空手动设置的值：
```C
if (scan_all_pmts_set)
    av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
```

5. av_format_inject_global_side_data(ic)，目前不知道有什么用。
6. 如果输入参数中有`-find_stream_info`，就查找stream的详细信息，但我试了之后发现输出好像并没什么区别。
7. 如果输入参数中有`-ss`，这个值会被解析到`start_time`，那么就会从这个地方开始解码。需要注意的是AVStream也会有个起始时间，这个值在AVFormatContext中，也就是最终的起始时间是`start_time + ic->start_time`。然后调用`avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0)`来跳转。
8. 如果在命令行中指定了`-status`参数，则`show_status`不为0，会调用`av_dump_format()`来打印媒体信息。
9. 如果在参数中指定了ast、vst以及sst，它们是选择期望的流，这样的话wanted_stream_spec就会有值，然后与st_index做一些运算。我们按照最简单直接输入文件名播放的行为，此处的wanted_stream_spec是没有值的，先略过。因此接下来st_index的值都为-1。
10. 如果允许video，调用
```C
st_index[AVMEDIA_TYPE_VIDEO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                                st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
```
寻找video的stream index。注意此时st_index[AVMEDIA_TYPE_VIDEO] = -1。
11. 如果允许audio，调用
```C
st_index[AVMEDIA_TYPE_AUDIO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                                st_index[AVMEDIA_TYPE_AUDIO],
                                st_index[AVMEDIA_TYPE_VIDEO],
                                NULL, 0);
```
寻找audio的stream index，其中st_index[AVMEDIA_TYPE_AUDIO] = -1，st_index[AVMEDIA_TYPE_VIDEO]在上一步video的步骤中已经获得。
12. 如果同时允许video和subsitle，按照同样的步骤查找subtitle的stream index。
```C
st_index[AVMEDIA_TYPE_SUBTITLE] =
            av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
                                st_index[AVMEDIA_TYPE_SUBTITLE],
                                (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
                                 st_index[AVMEDIA_TYPE_AUDIO] :
                                 st_index[AVMEDIA_TYPE_VIDEO]),
                                NULL, 0);
```
13. 将show_mode赋值给is->show_mode。
14. 如果输入流中包含video，那么就计算video的宽高，设置SDL的窗口大小。
15. 如果输入流中包含audio，调用`stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO])`打开音频流。该函数如果成功打开对应的stream，那么也会将stream index赋值给对应的is->audio_stream、is->video_stream和is->subtitle_stream。
16. 如果输入流中包含video，调用`stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO])`打开视频流。如果输入参数中没有指定show_mode，那么如果成功打开视频流，就使is->show_mode = SHOW_MODE_VIDEO，显示流中的视频，通常这个在播放包含专辑图片的音频文件时，就会导致ffplay打开窗口显示专辑封面；如果没有成功打开视频流，就使is->show_mode = SHOW_MODE_RDFT，显示音频的频谱（DFT是离散傅立叶变换）。
17. 如果输入流中包含subtitle，用同样的方式打开subtitle流。
18. 如果video和audio都没有成功打开，就退出。
19. 接下来进入循环，这个循环一直读取文件并将读取的AVPacket放到is中。非常重要的一点，要检查读取处的packet是否是在播放范围。这个播放范围有两种，一种是用户在参数中指定的播放起始点，另一种是stream自身的播放起始点。如果在播放范围中，就放入is的queue中，否则弃用。

## 2.10 stream_component_open

完整声明：
```C
/* open a given stream. Return 0 if OK */
static int stream_component_open(VideoState *is, int stream_index)
```

打开一个stream，并解码。

1. 调用`avcodec_alloc_context3(NULL)`新建一个AVCodecContext *avctx，调用`avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar)`向avctx中设置参数。最后调用`codec = avcodec_find_decoder(avctx->codec_id)`获取解码器。
2. 根据avctx->codec_type，将函数参数中的stream_index设置到is->last_audio_stream、is->last_subtitle_stream及is->last_video_stream。在运行ffplay时指定了-scodec、-acodec或-vcodec传入了解码器名称，则表明用户想强制使用一个解码器。此时为force_codec_name为用户指定的解码器名称。并调用`codec = avcodec_find_decoder_by_name(forced_codec_name)`来查找解码器。
3. 调用`avcodec_open2(avctx, codec, &opts)`打开解码器，opts为前面的一堆操作，不知道有什么用。
4. `ic->streams[stream_index]->discard = AVDISCARD_DEFAULT`
5. 如果当前流是音频流，获取采样率、声道数以及channel_layout，这个函数用来设置SDL的音频参数，它会找到输入与输出都兼容的音频参数。然后调用`decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread)`初始化解码器，它的作用起始就是初始化Decoder，也就是参数中的is->auddec。最后调用`decoder_start(&is->auddec, audio_thread, "audio_decoder", is)`开始解码，这个函数中，将audio_thread放在单独的线程中运行。
6. 对于video和subtitle都是和5一样的步骤。

## 2.11 audio_thread

完整声明：
```C
static int audio_thread(void *arg)
```


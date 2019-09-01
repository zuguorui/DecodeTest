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

void decodeAudio(const char *inputFileName, const char *outputFileName)
{
    int err = 0;
    AVFormatContext *formatContext = NULL;
    AVInputFormat *inputFormat = av_find_input_format(inputFileName);

    if(!inputFormat)
    {
        fprintf(stderr, "Can't detect the format of input file\n");
        exit(1);
    }

    formatContext = avformat_alloc_context();
    if(!formatContext)
    {
        fprintf(stderr, "Can't allocate context\n");
        exit(1);
    }

    err = avformat_open_input(&formatContext, inputFileName, inputFormat, NULL);
    if(err < 0)
    {
        fprintf(stderr, "Can't open input file\n");
        exit(1);
    }

    av_format_inject_global_side_data(formatContext);

    
    


}
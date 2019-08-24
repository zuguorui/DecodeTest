#ifndef WAVUTIL_H
#define WAVUTIL_H
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define WAV_HEADER_LEN 44

typedef struct
{
    char chunk_id[4];
    int32_t chunk_size;
    char format[4];
    char subchunk1_id[4];
    int32_t subchunk1_size;
    int16_t audio_format;
    int16_t num_channels;
    int32_t sample_rate; // sample_rate denotes the sampling rate.
    int32_t byte_rate;
    int16_t block_align;
    int16_t bits_per_sample;
    char subchunk2_id[4];
    int32_t subchunk2_size; // subchunk2_size denotes the number of samples.
} WavHeader;

void initWavHeader(WavHeader *wavHeader, uint64_t audioTotalLen, uint32_t sampleRate, uint32_t channelCount, uint32_t bitDepth);

#endif // WAVUTIL_H
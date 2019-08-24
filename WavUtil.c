#include "WavUtil.h"

/**
 * 获取wav头
 *
 * @param audioTotalLen: 整个音频PCM数据大小
 *
 * @param sampleRate: 采样率
 * @param channelCount: 声道数
 * @param bitDepth: 采样深度，即每个样本多少bit。
 * */

void initWavHeader(WavHeader *wavHeader, uint64_t audioTotalLen, uint32_t sampleRate, uint32_t channelCount, uint32_t bitDepth)
{
    int32_t byteRate = sampleRate * channelCount * bitDepth / 8;
    //总大小不包括"RIFF"和"WAVE"
    wavHeader->chunk_id[0] = 'R';
    wavHeader->chunk_id[1] = 'I';
    wavHeader->chunk_id[2] = 'F';
    wavHeader->chunk_id[3] = 'F';

    uint64_t totalDataLen = audioTotalLen + 44 - 8;
    wavHeader->chunk_size = totalDataLen;
    wavHeader->format[0] = 'W';
    wavHeader->format[1] = 'A';
    wavHeader->format[2] = 'V';
    wavHeader->format[3] = 'E';

    wavHeader->subchunk1_id[0] = 'f';
    wavHeader->subchunk1_id[1] = 'm';
    wavHeader->subchunk1_id[2] = 't';
    wavHeader->subchunk1_id[3] = ' ';

    wavHeader->subchunk1_size = 16;

    wavHeader->audio_format = 0x01;

    wavHeader->num_channels = (int16_t)channelCount;
    wavHeader->sample_rate = sampleRate;
    wavHeader->byte_rate = byteRate;

    int a = channelCount * bitDepth / 8;
    wavHeader->block_align = a;
    wavHeader->bits_per_sample = bitDepth;

    wavHeader->subchunk2_id[0] = 'd';
    wavHeader->subchunk2_id[1] = 'a';
    wavHeader->subchunk2_id[2] = 't';
    wavHeader->subchunk2_id[3] = 'a';
    wavHeader->subchunk2_size = audioTotalLen;
}
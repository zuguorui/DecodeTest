#ifndef _SIMPLE_DECODE_AUDIO_H_
#define _SIMPLE_DECODE_AUDIO_H_

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

class SimpleDecodeAudio{
public:
    static void decodeAudio(const char* inputFileName, const char* outputFileName);
};



#endif

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include "SimpleDecodeAudio.h"
#include "OfficalDecodeAudio.h"

using namespace std;



int main()
{
    // const char* in = "./audio_file/Mili - RTRT.mp3";
    const char* in = "./audio_file/张靓颖,王铮亮-只是没有如果.flac";
    const char* out = "./output.pcm";
    OfficalDecodeAudio::decodeAudio(in, out);
    // SimpleDecodeAudio::decodeAudio(in, out);
    


    return 0;
}

 

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
    const char* in = "./audio_file/fumika - Endless Road.mp3";
    // const char* in = "./audio_file/BAAD - 君が好きだと叫びたい.aac";
    const char* out = "./output.pcm";
    OfficalDecodeAudio::decodeAudio(in, out);
    // SimpleDecodeAudio::decodeAudio(in, out);
    


    return 0;
}

 

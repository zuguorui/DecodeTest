#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include "SimpleDecodeAudio.h"

using namespace std;



int main()
{
    const char* in = "./安来宁-我的名字叫做安.flac";
    const char* out = "./output.pcm";
    decodeAudio(in, out);


    return 0;
}

 

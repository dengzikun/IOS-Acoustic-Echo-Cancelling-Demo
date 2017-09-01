//
//  Opus_Decoder.hpp
//  Test_AEC
//
//  Created by ddp on 16/7/14.
//  Copyright © 2016年 ddp. All rights reserved.
//

#ifndef Opus_Decoder_hpp
#define Opus_Decoder_hpp

#include <stdio.h>
#include "include/opus/opus.h"

class Opus_Decoder{
public:
    Opus_Decoder(opus_int32 Fs,
                 int channels,
                 int *error);
    ~Opus_Decoder();
    
    int Decode( const unsigned char *data,
                opus_int32 len,
                opus_int16 *pcm,
                int frame_size,
               int decode_fec){
        return opus_decode(_decoder, data, len, pcm, frame_size, decode_fec);
    }
 
private:
    OpusDecoder *_decoder = nullptr;
};

#endif /* Opus_Decoder_hpp */

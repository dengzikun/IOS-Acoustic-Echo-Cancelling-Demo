//
//  Opus_Encoder.hpp
//  Test_AEC
//
//  Created by ddp on 16/7/14.
//  Copyright © 2016年 ddp. All rights reserved.
//

#ifndef Opus_Encoder_hpp
#define Opus_Encoder_hpp

#include <stdio.h>
#include "include/opus/opus.h"

class Opus_Encoder{
public:
    Opus_Encoder(opus_int32 Fs,
                 int channels,
                 int application,
                 int *error);
    ~Opus_Encoder();
    
    
    opus_int32 Encode( const opus_int16 *pcm,
                       int frame_size,
                       unsigned char *data,
                       opus_int32 max_data_bytes
                      ){
        
        return opus_encode(_encoder, pcm, frame_size, data, max_data_bytes);
    }
    
private:
    OpusEncoder *_encoder = nullptr;
};

#endif /* Opus_Encoder_hpp */

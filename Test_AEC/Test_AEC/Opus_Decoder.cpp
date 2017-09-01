//
//  Opus_Decoder.cpp
//  Test_AEC
//
//  Created by ddp on 16/7/14.
//  Copyright © 2016年 ddp. All rights reserved.
//

#include "Opus_Decoder.hpp"

Opus_Decoder::Opus_Decoder(opus_int32 Fs,
                           int channels,
                           int *error){
    _decoder = opus_decoder_create(Fs, channels, error);
}

Opus_Decoder::~Opus_Decoder(){
    opus_decoder_destroy(_decoder);
}
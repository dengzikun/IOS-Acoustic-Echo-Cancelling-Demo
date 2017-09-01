//
//  Opus_Encoder.cpp
//  Test_AEC
//
//  Created by ddp on 16/7/14.
//  Copyright © 2016年 ddp. All rights reserved.
//

#include "Opus_Encoder.hpp"

Opus_Encoder::Opus_Encoder(opus_int32 Fs,
                           int channels,
                           int application,
                           int *error){
    
    _encoder = opus_encoder_create(Fs, channels, application, error);
    if(*error != OPUS_OK)return ;
    
    opus_encoder_ctl(_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_int32 z = 1;
    opus_encoder_ctl(_encoder, OPUS_SET_DTX(z));
    opus_encoder_ctl(_encoder, OPUS_SET_PACKET_LOSS_PERC(20));
    //*/
}

Opus_Encoder::~Opus_Encoder(){
    opus_encoder_destroy(_encoder);
}
//
//  AudioMacros.h
//  Test_AEC
//
//  Created by ddp on 16/7/15.
//  Copyright © 2016年 ddp. All rights reserved.
//

#ifndef AudioMacros_h
#define AudioMacros_h

// audio general
#define AUDIO_SESSION_PREFERRED_IO_BUFFER_DURATION_MS 10
#define FRAMES_PER_PACKET 1
#define BITS_PER_CHANNEL 16
#define BYTES_PER_CHANNEL (BITS_PER_CHANNEL/8)

// audio recording
#define RECORDING_SAMPLE_RATE_HZ 16000 /* opus encoder input only support 8000, 12000, 16000, 24000, or 48000.*/
#define RECORDING_CHANNELS_PER_FRAME 1
#define RECORDING_BYTES_PER_FRAME (BYTES_PER_CHANNEL*RECORDING_CHANNELS_PER_FRAME)
#define RECORDING_BYTES_PER_PACKET (RECORDING_BYTES_PER_FRAME*FRAMES_PER_PACKET)
#define AUDIO_SESSION_PREFERRED_RECORDING_BUFFER_DURATION_BYTES (RECORDING_SAMPLE_RATE_HZ*AUDIO_SESSION_PREFERRED_IO_BUFFER_DURATION_MS/1000*RECORDING_BYTES_PER_FRAME)

// audio playback
#define PLAYBACK_SAMPLE_RATE_HZ 16000 /* opus decoder output only support 8000, 12000, 16000, 24000, or 48000.*/
#define PLAYBACK_CHANNELS_PER_FRAME 1
#define PLAYBACK_BYTES_PER_FRAME (BYTES_PER_CHANNEL*PLAYBACK_CHANNELS_PER_FRAME)
#define PLAYBACK_BYTES_PER_PACKET (PLAYBACK_BYTES_PER_FRAME*FRAMES_PER_PACKET)
#define AUDIO_SESSION_PREFERRED_PLAYBACK_BUFFER_DURATION_BYTES (PLAYBACK_SAMPLE_RATE_HZ*AUDIO_SESSION_PREFERRED_IO_BUFFER_DURATION_MS/1000*PLAYBACK_BYTES_PER_FRAME)

// audio codec
#define ENCODER_INPUT_SIZE_MS 10 /*opus encoder input only support 2.5, 5, 10, 20, 40 or 60 ms*/
#define ENCODER_INPUT_FRAMES (RECORDING_SAMPLE_RATE_HZ*ENCODER_INPUT_SIZE_MS/1000)
#define ENCODER_INPUT_BYTES (ENCODER_INPUT_FRAMES*RECORDING_BYTES_PER_FRAME)

#endif /* AudioMacros_h */

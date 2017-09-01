//
//  IosAudioController.m
//  Test_AEC
//
//  Created by ddp on 16/6/28.
//  Copyright © 2016年 ddp. All rights reserved.
//

#import "IosAudioController.h"
#import <AVFoundation/AVAudioSession.h>

#import "NetworkManager.h"
#import "AudioMacros.h"

#define USE_GRAPH 1

#define kOutputBus 0
#define kInputBus 1


void checkStatus(int status){
    if (status) {
        printf("Status not 0! %d\n", status);
        //		exit(1);
    }
}

volatile bool stop = false;

static OSStatus recordingCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                           const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                           UInt32 inNumberFrames,
                           AudioBufferList *ioData){
    
//    printf("recording: %u\n", inNumberFrames*RECORDING_BYTES_PER_FRAME);
    
    if(stop){
        return noErr;
    }
    
    OSStatus status = noErr;
    
    uint8_t *p;
    uint16_t length = [network getWritePointer:&p];
    UInt32 recordingBytes = inNumberFrames * RECORDING_BYTES_PER_FRAME;
    if(length >= recordingBytes){
        
        AudioBuffer buffer;
        buffer.mNumberChannels = RECORDING_CHANNELS_PER_FRAME;
        buffer.mDataByteSize = recordingBytes;
        buffer.mData = p;
        
        
        AudioBufferList bufferList;
        bufferList.mNumberBuffers = 1;
        bufferList.mBuffers[0] = buffer;
        
        IosAudioController* iosAudio = (__bridge IosAudioController *)(inRefCon);
        status = AudioUnitRender([iosAudio getAudioUnit], ioActionFlags, inTimeStamp, inBusNumber,
                                 inNumberFrames, &bufferList);
        
        [network didWriteLength: recordingBytes];
    }else{
        printf("drop ");
    }
    
    checkStatus(status);
    return status;
}


static OSStatus playbackCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                          const AudioTimeStamp *inTimeStamp,
                          UInt32 inBusNumber,
                          UInt32 inNumberFrames,
                          AudioBufferList *ioData){
    
//    printf("playback:   %lu\n", inNumberFrames*PLAYBACK_BYTES_PER_FRAME);
    if(stop){
        *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
        return noErr;
    }
    
    for(int i = 0; i < ioData->mNumberBuffers; ++i){
        
        UInt32 playbackBytes = inNumberFrames * PLAYBACK_BYTES_PER_FRAME;
        
        AudioBuffer buffer = ioData->mBuffers[i];
        
        uint8_t *p;
        uint16_t length = [network getReadPointer:&p];
        if(length >= playbackBytes){
            memcpy(buffer.mData, p, playbackBytes);
            [network didReadLength:playbackBytes];
        }else{
//            memset(buffer.mData, 0, playbackBytes);
            memcpy(buffer.mData, p, length);
            [network didReadLength:length];
            memset(buffer.mData+length, 0, playbackBytes - length);
        }
    }
    
    return noErr;
}



@interface IosAudioController(){
#if USE_GRAPH
    AUGraph _graph;
#endif
}
@property (readonly) AudioComponentInstance audioUnit;
@end

@implementation IosAudioController

@synthesize audioUnit;

-(AudioComponentInstance)getAudioUnit{
    return self.audioUnit;
}


-(instancetype)init{
    
    self = [super init];
    if(self){
  //*
        AVAudioSession *session = [AVAudioSession sharedInstance];
        
        NSError *error = nil;
        [session setCategory:AVAudioSessionCategoryPlayAndRecord error:&error];
        
        //        BOOL success = [sessionInstance overrideOutputAudioPort:AVAudioSessionPortOverrideSpeaker error:&error];
        
        [session setMode:AVAudioSessionModeVideoChat error:&error];
        
        
        [session setPreferredSampleRate:RECORDING_SAMPLE_RATE_HZ error:&error];
        
        NSTimeInterval bufferDuration = AUDIO_SESSION_PREFERRED_IO_BUFFER_DURATION_MS/1000.0f;
        [session setPreferredIOBufferDuration:bufferDuration error:&error];
        
        
        [session setActive:YES error:&error];

        NSTimeInterval bd = [session IOBufferDuration];
        double sr = [session sampleRate];
//*/
        
        OSStatus status;
        

        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;//kAudioUnitSubType_VoiceProcessingIO;//kAudioUnitSubType_RemoteIO;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;

#if USE_GRAPH
        AUNode outputNode;
        
        status = NewAUGraph(&_graph);

        status = AUGraphAddNode(_graph, &desc, &outputNode);
        
        status = AUGraphOpen(_graph);
        
        status = AUGraphNodeInfo(_graph, outputNode, NULL, &audioUnit);
        
#else
         AudioComponent inputComponent = AudioComponentFindNext(NULL, &desc);
         status = AudioComponentInstanceNew(inputComponent, &audioUnit);
         checkStatus(status);
#endif
        
        
        // recording
        UInt32 flag = 1;
        status = AudioUnitSetProperty(audioUnit,
                                      kAudioOutputUnitProperty_EnableIO,
                                      kAudioUnitScope_Input,
                                      kInputBus,
                                      &flag,
                                      sizeof(flag));
        
        checkStatus(status);
        
        // playback
        status = AudioUnitSetProperty(audioUnit,
                                      kAudioOutputUnitProperty_EnableIO,
                                      kAudioUnitScope_Output,
                                      kOutputBus,
                                      &flag,
                                      sizeof(flag));
        
        AudioStreamBasicDescription audioFormat;
        audioFormat.mFormatID = kAudioFormatLinearPCM;
        audioFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;
        
        audioFormat.mSampleRate = RECORDING_SAMPLE_RATE_HZ;
        audioFormat.mFramesPerPacket = FRAMES_PER_PACKET;
        audioFormat.mChannelsPerFrame = RECORDING_CHANNELS_PER_FRAME;
        audioFormat.mBitsPerChannel = BITS_PER_CHANNEL;
        audioFormat.mBytesPerFrame = RECORDING_BYTES_PER_FRAME;
        audioFormat.mBytesPerPacket = RECORDING_BYTES_PER_PACKET;
        audioFormat.mReserved = 0;
        
        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, kInputBus, &audioFormat, sizeof(audioFormat));
        
        checkStatus(status);
        
        audioFormat.mSampleRate = PLAYBACK_SAMPLE_RATE_HZ;
        audioFormat.mChannelsPerFrame = PLAYBACK_CHANNELS_PER_FRAME;
        audioFormat.mBytesPerFrame = PLAYBACK_BYTES_PER_FRAME;
        audioFormat.mBytesPerPacket = PLAYBACK_BYTES_PER_PACKET;
        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, kOutputBus, &audioFormat, sizeof(audioFormat));
        
        checkStatus(status);
        
        
        //UInt32 maxFramesPerSlice = 4096;
        //AudioUnitSetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFramesPerSlice, sizeof(UInt32));
        
        // Get the property value back from AURemoteIO. We are going to use this value to allocate buffers accordingly
        //        UInt32 propSize = sizeof(UInt32);
        //        AudioUnitGetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFramesPerSlice, &propSize);
        
        
        //Set input callback
        
        AURenderCallbackStruct callbackStruct;
        callbackStruct.inputProc = recordingCallback;
        callbackStruct.inputProcRefCon = (__bridge void * _Nullable)(self);
        
        status = AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Output, kInputBus, &callbackStruct, sizeof(callbackStruct));
        
        
        checkStatus(status);
        
        //Set output callback
        
        callbackStruct.inputProc = playbackCallback;
        callbackStruct.inputProcRefCon = (__bridge void * _Nullable)(self);
        
        
        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, kOutputBus, &callbackStruct, sizeof(callbackStruct));
        
        
        checkStatus(status);
        
        //        flag = 1;
        //        status = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_ShouldAllocateBuffer, kAudioUnitScope_Output, kInputBus, &flag, sizeof(flag));
        
        
      
#if USE_GRAPH
        status = AUGraphInitialize(_graph);
#else
        status = AudioUnitInitialize(audioUnit);
#endif
        checkStatus(status);
    }
    
    return self;
}

-(void)start{
#if USE_GRAPH
    OSStatus status = AUGraphStart(_graph);
#else
    OSStatus status = AudioOutputUnitStart(audioUnit);
#endif
    checkStatus(status);
    
    stop = false;
}

-(void)stop{
    stop = true;
#if USE_GRAPH
    OSStatus status = AUGraphStop(_graph);
#else
    OSStatus status = AudioOutputUnitStop(audioUnit);
#endif
    checkStatus(status);
}

-(BOOL)isAECOn{
    UInt32 echoCancellation;
    UInt32 size = sizeof(echoCancellation);
    AudioUnitGetProperty(audioUnit,
                         kAUVoiceIOProperty_BypassVoiceProcessing,
                         kAudioUnitScope_Global,
                         0,
                         &echoCancellation,
                         &size);
    if (echoCancellation==0) {
        return YES;
    }else{
        return NO;
    }
}

-(void)setAECOn: (BOOL)isOn{
    UInt32 echoCancellation;
    if(isOn){
        echoCancellation = 0;
    }else{
        echoCancellation = 1;
    }
    
    AudioUnitSetProperty(self.audioUnit,
                         kAUVoiceIOProperty_BypassVoiceProcessing,
                         kAudioUnitScope_Global,
                         0,
                         &echoCancellation,
                         sizeof(echoCancellation));
}

-(BOOL)isAGCOn{
    UInt32 agc;
    UInt32 size = sizeof(agc);
    AudioUnitGetProperty(audioUnit,
                         kAUVoiceIOProperty_VoiceProcessingEnableAGC,
                         kAudioUnitScope_Global,
                         0,
                         &agc,
                         &size);
    if (agc==1) {
        return YES;
    }else{
        return NO;
    }
}

-(void)setAGCOn: (BOOL)isOn{
    UInt32 agc;
    if(isOn){
        agc = 1;
    }else{
        agc = 0;
    }
    
    AudioUnitSetProperty(self.audioUnit,
                         kAUVoiceIOProperty_VoiceProcessingEnableAGC,
                         kAudioUnitScope_Global,
                         0,
                         &agc,
                         sizeof(agc));
}


-(void)dealloc{
#if USE_GRAPH
    DisposeAUGraph(_graph);
#else
    AudioComponentInstanceDispose(audioUnit);
#endif
}

@end

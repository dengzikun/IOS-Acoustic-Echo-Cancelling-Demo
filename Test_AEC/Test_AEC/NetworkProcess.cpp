//
//  NetworkProcess.cpp
//  Test_AEC
//
//  Created by ddp on 16/7/4.
//  Copyright © 2016年 ddp. All rights reserved.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <sys/fcntl.h>
#include "IPAddress.h"

#include "NetworkProcess.hpp"

#include "AudioMacros.h"

using namespace std::chrono;

NetworkProcess::NetworkProcess(string &targetIP):_targetIP(targetIP)
{    
   _rcvBufferManager.reset(new VirtualRingBuffer(AUDIO_SESSION_PREFERRED_PLAYBACK_BUFFER_DURATION_BYTES*1)); // VRB alloc align 4K page
   _sndBufferManager.reset(new VirtualRingBuffer(AUDIO_SESSION_PREFERRED_RECORDING_BUFFER_DURATION_BYTES*1)); // VRB alloc align 4K page
    
    int error;
    _encoder.reset(new Opus_Encoder(RECORDING_SAMPLE_RATE_HZ, RECORDING_CHANNELS_PER_FRAME, OPUS_APPLICATION_VOIP, &error));
    if(error != OPUS_OK)return ;
    _decoder.reset(new Opus_Decoder(PLAYBACK_SAMPLE_RATE_HZ, PLAYBACK_CHANNELS_PER_FRAME, &error));
    if(error != OPUS_OK)return ;
    
    InitNetwork();
    
    _rcvThread.reset(new thread(&NetworkProcess::RcvThread, this));
    _sndThread.reset(new thread(&NetworkProcess::SndThread, this));
}

NetworkProcess::~NetworkProcess(){
    _sndDone = true;
    _rcvDone = true;
    
    shutdown(_socket, SHUT_RDWR);
    close(_socket);
    
    if(_sndThread && _sndThread->joinable()){
        _sndThread->join();
    }
    
    if(_rcvThread && _rcvThread->joinable()){
        _rcvThread->join();
    }
}

void NetworkProcess::InitNetwork(){
    _socket = socket(AF_INET, SOCK_DGRAM, 0);
    
//    int flags = fcntl(_socket, F_GETFL);
//    flags |= O_NONBLOCK;
//    fcntl(_socket, F_SETFL, flags);
    
    int reuseAddr = 1;
    if (-1 == setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, (const char *) &reuseAddr,
                         sizeof(int))) {
        printf("set SO_REUSEADDR failed\n");
    }
    
    socklen_t addrLen;
    struct sockaddr_in socketAddr;
    addrLen = sizeof(struct sockaddr_in);
    bzero(&socketAddr, addrLen);
    
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    socketAddr.sin_port = htons(13000);
    
    int ret = ::bind(_socket, (struct sockaddr*)&socketAddr, addrLen);
    if(ret < 0) {
        printf("bind socket fail: %d\n", errno);
    }
}

void NetworkProcess::RcvThread(){

    socklen_t addrLen;
    struct sockaddr_in socketAddr;
    
    #define BUFFER_SIZE (4*1024)

    unique_ptr<uint8_t[]> rcvBuffer(new uint8_t[BUFFER_SIZE]);
    unique_ptr<opus_int16[]> pcm(new opus_int16[BUFFER_SIZE/2]);
    while(!_rcvDone){
        
        ssize_t len = recvfrom(_socket, rcvBuffer.get(), BUFFER_SIZE, 0, (struct sockaddr *)&socketAddr, &addrLen);
        if(len != -1){
            uint32_t currentSN = (rcvBuffer.get()[0] << 24)|(rcvBuffer.get()[1] << 16)|(rcvBuffer.get()[2] << 8)|rcvBuffer.get()[3];
            if(IsPacketLoss(currentSN)){
                int frames = _decoder->Decode(nullptr, 0, pcm.get(), BUFFER_SIZE/PLAYBACK_BYTES_PER_FRAME, 1);
                if(frames > 0){
                    int32_t decodedBytes = frames*PLAYBACK_BYTES_PER_FRAME;
                    int32_t availableBytes;
                    uint8_t *buf = _rcvBufferManager->GetWritePointer(&availableBytes);
                    if(availableBytes >= decodedBytes){
                        memcpy(buf, pcm.get(), decodedBytes);
                        _rcvBufferManager->DidWrite(decodedBytes);
                    }
                }
            }
            int frames = _decoder->Decode(rcvBuffer.get()+4, len - 4, pcm.get(), BUFFER_SIZE/PLAYBACK_BYTES_PER_FRAME, 0);
            if(frames > 0){
                int32_t decodedBytes = frames*PLAYBACK_BYTES_PER_FRAME;
                int32_t availableBytes;
                uint8_t *buf = _rcvBufferManager->GetWritePointer(&availableBytes);
                if(availableBytes >= decodedBytes){
                    memcpy(buf, pcm.get(), decodedBytes);
                    _rcvBufferManager->DidWrite(decodedBytes);
                }
            }
            CalcLostRate(currentSN);
        }
    }
}

void NetworkProcess::SndThread(){
    
    socklen_t addrLen;
    struct sockaddr_in sockAddr;
    
    addrLen = sizeof(struct sockaddr_in);
    bzero(&sockAddr, addrLen);
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(_targetIP.c_str());
    sockAddr.sin_port = htons(13000);
    
    unique_ptr<uint8_t[]> output(new uint8_t[ENCODER_INPUT_BYTES+4]);
    while(!_sndDone){
        
        int32_t availableBytes;
        uint8_t *input = _sndBufferManager->GetReadPointer(&availableBytes);
        if(availableBytes >= ENCODER_INPUT_BYTES){
            
            opus_int32 encodedBytes = _encoder->Encode((const opus_int16*)input, ENCODER_INPUT_FRAMES, output.get()+4, ENCODER_INPUT_BYTES);
            _sndBufferManager->DidRead(ENCODER_INPUT_BYTES);
            if(encodedBytes > 0){
                output.get()[0] = _sndCount >> 24;
                output.get()[1] = _sndCount >> 16;
                output.get()[2] = _sndCount >> 8;
                output.get()[3] = _sndCount ;
                
                ssize_t len = sendto(_socket, output.get(), encodedBytes + 4, 0, (struct sockaddr *)&sockAddr, addrLen);
                if(len == -1){
                    printf("sendto failed: %d\n", errno);
                }
                
                _sndCount++;
            }
        }else{
            _sndBufferManager->ReadWait(20);
        }
    }
}

uint16_t NetworkProcess::getWritePointer(uint8_t **buf){
    int32_t availableBytes;
    *buf = _sndBufferManager->GetWritePointer(&availableBytes);
    return availableBytes;
}

void NetworkProcess::didWriteLength(uint16_t length){
    _sndBufferManager->DidWrite(length);
}


uint16_t NetworkProcess::getReadPointer(uint8_t **buf){
    int32_t availableBytes;
    *buf = _rcvBufferManager->GetReadPointer(&availableBytes);
    return availableBytes;
}

void NetworkProcess::didReadLength(uint16_t length){
    _rcvBufferManager->DidRead(length);
}

bool NetworkProcess::IsPacketLoss(uint32_t currentSN){
    bool bLost = false;
    if(_rcvCount != -1){
        if(currentSN <= _rcvCount){
            return bLost;
        }
        uint32_t diff = currentSN - _rcvCount;
        if(diff != 1){
            bLost = true;
        }
    }
    return bLost;
}

void NetworkProcess::CalcLostRate(uint32_t currentSN){
    if(_rcvCount != -1){
        if(currentSN <= _rcvCount){
            _rcvCount = currentSN - 1;
        }
        uint32_t diff = currentSN - _rcvCount;
        if(diff != 1){
            _dropCount += (diff-1);
            _totalDropCount += (diff - 1);
        }
    }
    _rcvCount = currentSN;
    
    if(_rcvPrevCount == -1){
        _rcvPrevCount = _rcvCount;
    }
    
    static time_point<high_resolution_clock> last = high_resolution_clock::now();
    
    time_point<high_resolution_clock> now = high_resolution_clock::now();
    
    int32_t diff = (int32_t)duration_cast<chrono::milliseconds>(now - last).count();
    if(diff >= 2000){
        
        if(_dropCount == 0){
            _lostRate = 0;
        }else if(_rcvCount - _rcvPrevCount != 0){
            _lostRate = _dropCount * 1.0f / (_rcvCount - _rcvPrevCount);
        }
        //            printf("dropRate: %.2f\n", dropCount * 1.0f / (rcvCount - rcvPrevCount));
        
        _dropCount = 0;
        _rcvPrevCount = _rcvCount;
        last = now;
    }
    ++_rcvTotalCount;
}

uint32_t NetworkProcess::GetTotalDrop(){
    return _totalDropCount;
}

double NetworkProcess::GetLostRate(){
    return _lostRate;
}

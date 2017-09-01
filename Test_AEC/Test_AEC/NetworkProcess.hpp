//
//  NetworkProcess.hpp
//  Test_AEC
//
//  Created by ddp on 16/7/4.
//  Copyright © 2016年 ddp. All rights reserved.
//

#ifndef NetworkProcess_hpp
#define NetworkProcess_hpp

#include <stdio.h>
#include <thread>
#include <memory>
#include <string>
#include "SyncQueue.h"
#include "VirtualRingBuffer.hpp"
#include "Opus_Encoder.hpp"
#include "Opus_Decoder.hpp"

using namespace std;


class NetworkProcess{
public:
    NetworkProcess(string& targetIP);
    ~NetworkProcess();
    
    void PushData(uint8_t *data, uint16_t length);

    uint16_t getWritePointer(uint8_t **buf);
    void     didWriteLength(uint16_t length);
    
    uint16_t getReadPointer(uint8_t **buf);
    void     didReadLength(uint16_t length);
    
    double GetLostRate();
    uint32_t GetTotalDrop();
    uint32_t GetTotalRcvCount(){
        return _rcvTotalCount;
    }
    
private:
    void InitNetwork();
    void CalcLostRate(uint32_t currentSN);
    bool IsPacketLoss(uint32_t currentSN);
    void RcvThread();
    void SndThread();
    
private:
    unique_ptr<thread> _rcvThread;
    unique_ptr<thread> _sndThread;
    bool _rcvDone = false;
    bool _sndDone = false;
    
    int _socket = -1;
    
    unique_ptr<VirtualRingBuffer> _rcvBufferManager;
    unique_ptr<VirtualRingBuffer> _sndBufferManager;
    
    unique_ptr<Opus_Encoder> _encoder;
    unique_ptr<Opus_Decoder> _decoder;
    
    
    string _targetIP;
    
    uint32_t _rcvTotalCount = 0;
    uint32_t _rcvCount = -1;
    uint32_t _dropCount = 0;
    
    uint32_t _rcvPrevCount = -1;
    
    double _lostRate = 0;
    
    uint32_t _sndCount = 0;
    
    uint32_t _totalDropCount = 0;
};

#endif /* NetworkProcess_hpp */
